#include "vkit/workflow/serializer.hpp"

#include <simdjson.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_map>

#include "vkit/workflow/node/material/diffuse.hpp"
#include "vkit/workflow/node/material/diffuse_specular.hpp"
#include "vkit/workflow/node/material/mix_material.hpp"
#include "vkit/workflow/node/material/principled_bsdf.hpp"
#include "vkit/workflow/node/material/slot_output.hpp"
#include "vkit/workflow/node/operators/channel_adjust.hpp"
#include "vkit/workflow/node/operators/channel_remap.hpp"
#include "vkit/workflow/node/operators/heightmap.hpp"
#include "vkit/workflow/node/operators/mix.hpp"
#include "vkit/workflow/node/operators/normalmap.hpp"
#include "vkit/workflow/node/operators/sobel.hpp"
#include "vkit/workflow/node/operators/tint.hpp"
#include "vkit/workflow/node/procedural/fractal_generator.hpp"
#include "vkit/workflow/node/procedural/noise_generator.hpp"
#include "vkit/workflow/node/procedural/pattern_generator.hpp"
#include "vkit/workflow/node/texture_load.hpp"

namespace vkit::workflow {

namespace {

// ── JSON writing helpers ──────────────────────────────────────────────────────

static std::string flt(float v) {
  if (std::isinf(v))
    return v > 0.0f ? "3.4028235e+38" : "-3.4028235e+38";
  if (std::isnan(v)) return "0";
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.7g", static_cast<double>(v));
  return buf;
}

static std::string esc(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

static std::string v2(const glm::vec2& v) {
  return "[" + flt(v.x) + "," + flt(v.y) + "]";
}
static std::string v3(const glm::vec3& v) {
  return "[" + flt(v.x) + "," + flt(v.y) + "," + flt(v.z) + "]";
}
static std::string v4(const glm::vec4& v) {
  return "[" + flt(v.x) + "," + flt(v.y) + "," + flt(v.z) + "," + flt(v.w) + "]";
}

// ── Per-node-type serialization ───────────────────────────────────────────────

static void serializeNodeFields(std::ostringstream& j, WorkflowNode* wn,
                                const std::filesystem::path& texDir) {
  namespace fs = std::filesystem;

  if (auto* n = dynamic_cast<node::TextureLoadNode*>(wn)) {
    j << "      \"type\": \"TextureLoad\",\n";
    std::string relPath;
    const auto& srcPath = n->getPath();
    if (!srcPath.empty() && fs::exists(srcPath)) {
      std::error_code ec;
      fs::create_directories(texDir, ec);
      auto destPath = texDir / srcPath.filename();
      fs::copy_file(srcPath, destPath, fs::copy_options::overwrite_existing, ec);
      if (!ec) relPath = "textures/" + srcPath.filename().string();
      else relPath = srcPath.string();
    } else {
      relPath = srcPath.string();
    }
    j << "      \"path\": \"" << esc(relPath) << "\",\n";
    j << "      \"useMipmaps\": " << (n->getUseMipmaps() ? "true" : "false") << "\n";

  } else if (auto* n = dynamic_cast<node::proc::FractalGeneratorNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"FractalGenerator\",\n";
    j << "      \"fractalType\": " << static_cast<uint32_t>(p.type) << ",\n";
    j << "      \"width\": " << p.width << ",\n";
    j << "      \"height\": " << p.height << ",\n";
    j << "      \"maxIterations\": " << p.maxIterations << ",\n";
    j << "      \"centerX\": " << flt(p.centerX) << ",\n";
    j << "      \"centerY\": " << flt(p.centerY) << ",\n";
    j << "      \"zoom\": " << flt(p.zoom) << ",\n";
    j << "      \"juliaRe\": " << flt(p.juliaRe) << ",\n";
    j << "      \"juliaIm\": " << flt(p.juliaIm) << "\n";

  } else if (auto* n = dynamic_cast<node::proc::NoiseGeneratorNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"NoiseGenerator\",\n";
    j << "      \"noiseType\": " << static_cast<uint32_t>(p.type) << ",\n";
    j << "      \"width\": " << p.width << ",\n";
    j << "      \"height\": " << p.height << ",\n";
    j << "      \"scale\": " << flt(p.scale) << ",\n";
    j << "      \"offsetX\": " << flt(p.offsetX) << ",\n";
    j << "      \"offsetY\": " << flt(p.offsetY) << ",\n";
    j << "      \"seed\": " << flt(p.seed) << ",\n";
    j << "      \"octaves\": " << p.octaves << ",\n";
    j << "      \"persistence\": " << flt(p.persistence) << ",\n";
    j << "      \"lacunarity\": " << flt(p.lacunarity) << ",\n";
    j << "      \"worleyMode\": " << static_cast<uint32_t>(p.worleyMode) << ",\n";
    j << "      \"worleyJitter\": " << flt(p.worleyJitter) << "\n";

  } else if (auto* n = dynamic_cast<node::proc::PatternGeneratorNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"PatternGenerator\",\n";
    j << "      \"width\": " << p.width << ",\n";
    j << "      \"height\": " << p.height << ",\n";
    j << "      \"patternType\": " << static_cast<uint32_t>(p.type) << ",\n";
    j << "      \"scale\": " << flt(p.scale) << ",\n";
    j << "      \"thickness\": " << flt(p.thickness) << ",\n";
    j << "      \"smoothness\": " << flt(p.smoothness) << ",\n";
    j << "      \"param1\": " << flt(p.param1) << ",\n";
    j << "      \"param2\": " << flt(p.param2) << "\n";

  } else if (auto* n = dynamic_cast<node::op::SobelNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"Sobel\",\n";
    j << "      \"intensity\": " << flt(p.intensity) << ",\n";
    j << "      \"threshold\": " << flt(p.threshold) << "\n";

  } else if (auto* n = dynamic_cast<node::op::HeightMapNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"HeightMap\",\n";
    j << "      \"contrast\": " << flt(p.contrast) << ",\n";
    j << "      \"brightness\": " << flt(p.brightness) << ",\n";
    j << "      \"invert\": " << p.invert << "\n";

  } else if (auto* n = dynamic_cast<node::op::NormalMapNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"NormalMap\",\n";
    j << "      \"strength\": " << flt(p.strength) << ",\n";
    j << "      \"invertX\": " << p.invertX << ",\n";
    j << "      \"invertY\": " << p.invertY << "\n";

  } else if (auto* n = dynamic_cast<node::op::TintNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"Tint\",\n";
    j << "      \"color\": [" << flt(p.color[0]) << "," << flt(p.color[1]) << ","
      << flt(p.color[2]) << "," << flt(p.color[3]) << "],\n";
    j << "      \"factor\": " << flt(p.factor) << ",\n";
    j << "      \"tintMode\": " << static_cast<uint32_t>(p.mode) << "\n";

  } else if (auto* n = dynamic_cast<node::op::MixNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"Mix\",\n";
    j << "      \"width\": " << p.width << ",\n";
    j << "      \"height\": " << p.height << ",\n";
    j << "      \"factor\": " << flt(p.factor) << ",\n";
    j << "      \"mixMode\": " << static_cast<uint32_t>(p.mode) << "\n";

  } else if (auto* n = dynamic_cast<node::op::ChannelRemapNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"ChannelRemap\",\n";
    j << "      \"outR\": " << p.outR << ",\n";
    j << "      \"outG\": " << p.outG << ",\n";
    j << "      \"outB\": " << p.outB << ",\n";
    j << "      \"outA\": " << p.outA << "\n";

  } else if (auto* n = dynamic_cast<node::op::ChannelAdjustNode*>(wn)) {
    const auto& p = n->getParams();
    j << "      \"type\": \"ChannelAdjust\",\n";
    j << "      \"gainR\": " << flt(p.gain[0]) << ",\n";
    j << "      \"gainG\": " << flt(p.gain[1]) << ",\n";
    j << "      \"gainB\": " << flt(p.gain[2]) << ",\n";
    j << "      \"gainA\": " << flt(p.gain[3]) << ",\n";
    j << "      \"biasR\": " << flt(p.bias[0]) << ",\n";
    j << "      \"biasG\": " << flt(p.bias[1]) << ",\n";
    j << "      \"biasB\": " << flt(p.bias[2]) << ",\n";
    j << "      \"biasA\": " << flt(p.bias[3]) << ",\n";
    j << "      \"invertR\": " << (p.invert[0] ? 1 : 0) << ",\n";
    j << "      \"invertG\": " << (p.invert[1] ? 1 : 0) << ",\n";
    j << "      \"invertB\": " << (p.invert[2] ? 1 : 0) << ",\n";
    j << "      \"invertA\": " << (p.invert[3] ? 1 : 0) << "\n";

  } else if (auto* n = dynamic_cast<node::mat::DiffuseNode*>(wn)) {
    j << "      \"type\": \"Diffuse\",\n";
    j << "      \"alphaMode\": " << static_cast<int>(n->alphaMode) << ",\n";
    j << "      \"diffuseFactor\": " << v4(n->diffuseFactor) << "\n";

  } else if (auto* n = dynamic_cast<node::mat::DiffuseSpecularNode*>(wn)) {
    j << "      \"type\": \"DiffuseSpecular\",\n";
    j << "      \"alphaMode\": " << static_cast<int>(n->alphaMode) << ",\n";
    j << "      \"diffuseFactor\": " << v4(n->diffuseFactor) << ",\n";
    j << "      \"specularFactor\": " << v3(n->specularFactor) << ",\n";
    j << "      \"glossinessFactor\": " << flt(n->glossinessFactor) << "\n";

  } else if (auto* n = dynamic_cast<node::mat::PrincipledBSDFNode*>(wn)) {
    j << "      \"type\": \"PrincipledBSDF\",\n";
    j << "      \"alphaMode\": " << static_cast<int>(n->alphaMode) << ",\n";
    j << "      \"baseColorFactor\": " << v4(n->baseColorFactor) << ",\n";
    j << "      \"emissiveFactor\": " << v3(n->emissiveFactor) << ",\n";
    j << "      \"metallicFactor\": " << flt(n->metallicFactor) << ",\n";
    j << "      \"roughnessFactor\": " << flt(n->roughnessFactor) << ",\n";
    j << "      \"occlusionStrength\": " << flt(n->occlusionStrength) << ",\n";
    j << "      \"ior\": " << flt(n->ior) << ",\n";
    j << "      \"specularColorFactor\": " << v3(n->specularColorFactor) << ",\n";
    j << "      \"specularFactor\": " << flt(n->specularFactor) << ",\n";
    j << "      \"transmissionFactor\": " << flt(n->transmissionFactor) << ",\n";
    j << "      \"thicknessFactor\": " << flt(n->thicknessFactor) << ",\n";
    j << "      \"attenuationDistance\": " << flt(n->attenuationDistance) << ",\n";
    j << "      \"attenuationColor\": " << v3(n->attenuationColor) << ",\n";
    j << "      \"sheenColorFactor\": " << v3(n->sheenColorFactor) << ",\n";
    j << "      \"sheenRoughnessFactor\": " << flt(n->sheenRoughnessFactor) << ",\n";
    j << "      \"clearcoatFactor\": " << flt(n->clearcoatFactor) << ",\n";
    j << "      \"clearcoatRoughnessFactor\": " << flt(n->clearcoatRoughnessFactor) << ",\n";
    j << "      \"anisotropyStrength\": " << flt(n->anisotropyStrength) << ",\n";
    j << "      \"anisotropyRotation\": " << v2(n->anisotropyRotation) << ",\n";
    j << "      \"iridescenceFactor\": " << flt(n->iridescenceFactor) << ",\n";
    j << "      \"iridescenceIor\": " << flt(n->iridescenceIor) << ",\n";
    j << "      \"iridescenceThicknessMin\": " << flt(n->iridescenceThicknessMin) << ",\n";
    j << "      \"iridescenceThicknessMax\": " << flt(n->iridescenceThicknessMax) << "\n";

  } else if (auto* n = dynamic_cast<node::mat::MixMaterialNode*>(wn)) {
    j << "      \"type\": \"MixMaterial\",\n";
    j << "      \"factor\": " << flt(n->factor) << ",\n";
    j << "      \"threshold\": " << flt(n->threshold) << ",\n";
    j << "      \"edge\": " << flt(n->edge) << ",\n";
    j << "      \"alphaCutoff\": " << flt(n->alphaCutoff) << ",\n";
    j << "      \"alphaMode\": " << static_cast<int>(n->alphaMode) << "\n";

  } else if (auto* n = dynamic_cast<node::mat::SlotOutputNode*>(wn)) {
    j << "      \"type\": \"SlotOutput\",\n";
    j << "      \"targetSlotId\": " << n->targetSlotId << "\n";

  } else {
    j << "      \"type\": \"Unknown\"\n";
  }
}

// ── Simdjson helpers ──────────────────────────────────────────────────────────

static float getF(simdjson::dom::object& obj, std::string_view key, float def) {
  double v;
  if (obj[key].get_double().get(v)) return def;
  return static_cast<float>(v);
}

static uint32_t getU32(simdjson::dom::object& obj, std::string_view key,
                       uint32_t def) {
  uint64_t v;
  if (obj[key].get_uint64().get(v)) return def;
  return static_cast<uint32_t>(v);
}

static int32_t getI32(simdjson::dom::object& obj, std::string_view key,
                      int32_t def) {
  int64_t v;
  if (obj[key].get_int64().get(v)) return def;
  return static_cast<int32_t>(v);
}

static bool getBool(simdjson::dom::object& obj, std::string_view key, bool def) {
  bool v;
  if (obj[key].get_bool().get(v)) return def;
  return v;
}

static glm::vec4 getVec4(simdjson::dom::object& obj, std::string_view key,
                          const glm::vec4& def) {
  simdjson::dom::array arr;
  if (obj[key].get_array().get(arr)) return def;
  float vals[4] = {def.x, def.y, def.z, def.w};
  int i = 0;
  for (auto el : arr) {
    if (i >= 4) break;
    double v; if (!el.get_double().get(v)) vals[i] = static_cast<float>(v);
    ++i;
  }
  return {vals[0], vals[1], vals[2], vals[3]};
}

static glm::vec3 getVec3(simdjson::dom::object& obj, std::string_view key,
                          const glm::vec3& def) {
  simdjson::dom::array arr;
  if (obj[key].get_array().get(arr)) return def;
  float vals[3] = {def.x, def.y, def.z};
  int i = 0;
  for (auto el : arr) {
    if (i >= 3) break;
    double v; if (!el.get_double().get(v)) vals[i] = static_cast<float>(v);
    ++i;
  }
  return {vals[0], vals[1], vals[2]};
}

static glm::vec2 getVec2(simdjson::dom::object& obj, std::string_view key,
                          const glm::vec2& def) {
  simdjson::dom::array arr;
  if (obj[key].get_array().get(arr)) return def;
  float vals[2] = {def.x, def.y};
  int i = 0;
  for (auto el : arr) {
    if (i >= 2) break;
    double v; if (!el.get_double().get(v)) vals[i] = static_cast<float>(v);
    ++i;
  }
  return {vals[0], vals[1]};
}

}  // namespace

// ── Export ────────────────────────────────────────────────────────────────────

bool WorkflowSerializer::exportToFile(
    Workflow* workflow,
    const std::vector<controller::NodePosition>& positions,
    const std::filesystem::path& filePath) {
  if (!workflow) return false;

  std::unordered_map<int, const controller::NodePosition*> posMap;
  posMap.reserve(positions.size());
  for (const auto& p : positions) posMap[p.nodeId] = &p;

  auto texDir = filePath.parent_path() / "textures";

  std::ostringstream j;
  j << "{\n";
  j << "  \"version\": 1,\n";
  j << "  \"nodes\": [\n";

  bool firstNode = true;
  for (auto* nodeBase : workflow->getNodes()) {
    auto* wn = static_cast<WorkflowNode*>(nodeBase);
    if (!firstNode) j << ",\n";
    firstNode = false;

    float x = 0.0f, y = 0.0f;
    if (auto it = posMap.find(wn->getId()); it != posMap.end()) {
      x = it->second->x;
      y = it->second->y;
    }

    j << "    {\n";
    j << "      \"id\": " << wn->getId() << ",\n";
    j << "      \"name\": \"" << esc(std::string(wn->getName())) << "\",\n";
    j << "      \"x\": " << flt(x) << ",\n";
    j << "      \"y\": " << flt(y) << ",\n";
    serializeNodeFields(j, wn, texDir);
    j << "    }";
  }

  j << "\n  ],\n";
  j << "  \"links\": [\n";

  bool firstLink = true;
  for (const auto& linkPtr : workflow->getLinks()) {
    auto* link = linkPtr.get();
    auto* srcPin  = link->getSrc();
    auto* sinkPin = link->getSink();
    if (!srcPin || !sinkPin) continue;

    if (!firstLink) j << ",\n";
    firstLink = false;

    j << "    {\n";
    j << "      \"srcNodeId\": " << srcPin->getOwnerNode()->getId() << ",\n";
    j << "      \"srcSlot\": " << srcPin->getSlot() << ",\n";
    j << "      \"sinkNodeId\": " << sinkPin->getOwnerNode()->getId() << ",\n";
    j << "      \"sinkSlot\": " << sinkPin->getSlot() << "\n";
    j << "    }";
  }

  j << "\n  ]\n";
  j << "}\n";

  std::ofstream file(filePath);
  if (!file) return false;
  file << j.str();
  return true;
}

// ── Import ────────────────────────────────────────────────────────────────────

bool WorkflowSerializer::importFromFile(
    const std::filesystem::path& filePath,
    controller::WorkflowController* controller,
    std::vector<controller::NodePosition>& outPositions) {
  if (!controller) return false;

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  if (auto err = parser.load(filePath.string()).get(doc)) return false;

  controller->clearWorkflow();

  namespace fs = std::filesystem;
  auto jsonDir = filePath.parent_path();

  std::unordered_map<int64_t, graph::Node*> idMap;

  simdjson::dom::array nodesArr;
  if (doc["nodes"].get_array().get(nodesArr)) return false;

  for (auto nodeEl : nodesArr) {
    simdjson::dom::object obj;
    if (nodeEl.get_object().get(obj)) continue;

    std::string_view type, name;
    int64_t oldId = 0;
    double x = 0.0, y = 0.0;

    if (obj["type"].get_string().get(type)) continue;
    if (obj["name"].get_string().get(name)) name = "";
    if (obj["id"].get_int64().get(oldId)) continue;
    (void)obj["x"].get_double().get(x);
    (void)obj["y"].get_double().get(y);

    std::string nameStr{name};
    WorkflowNode* newNode = nullptr;

    if (type == "TextureLoad") {
      auto* n = controller->createTextureLoadNode(nameStr);
      if (!n) continue;
      std::string_view path;
      if (!obj["path"].get_string().get(path)) {
        fs::path absPath = jsonDir / fs::path(std::string(path));
        n->setPath(absPath);
      }
      n->setUseMipmaps(getBool(obj, "useMipmaps", true));
      newNode = n;

    } else if (type == "FractalGenerator") {
      auto* n = controller->createFractalGeneratorNode(nameStr);
      if (!n) continue;
      node::proc::FractalParams p = n->getParams();
      p.type = static_cast<node::proc::FractalType>(getU32(obj, "fractalType", 0));
      p.width = getU32(obj, "width", p.width);
      p.height = getU32(obj, "height", p.height);
      p.maxIterations = getI32(obj, "maxIterations", p.maxIterations);
      p.centerX = getF(obj, "centerX", p.centerX);
      p.centerY = getF(obj, "centerY", p.centerY);
      p.zoom = getF(obj, "zoom", p.zoom);
      p.juliaRe = getF(obj, "juliaRe", p.juliaRe);
      p.juliaIm = getF(obj, "juliaIm", p.juliaIm);
      n->setParams(p);
      newNode = n;

    } else if (type == "NoiseGenerator") {
      auto* n = controller->createNoiseGeneratorNode(nameStr);
      if (!n) continue;
      node::proc::NoiseParams p = n->getParams();
      p.type = static_cast<node::proc::NoiseType>(getU32(obj, "noiseType", 0));
      p.width = getU32(obj, "width", p.width);
      p.height = getU32(obj, "height", p.height);
      p.scale = getF(obj, "scale", p.scale);
      p.offsetX = getF(obj, "offsetX", p.offsetX);
      p.offsetY = getF(obj, "offsetY", p.offsetY);
      p.seed = getF(obj, "seed", p.seed);
      p.octaves = getI32(obj, "octaves", p.octaves);
      p.persistence = getF(obj, "persistence", p.persistence);
      p.lacunarity = getF(obj, "lacunarity", p.lacunarity);
      p.worleyMode = static_cast<node::proc::WorleyMode>(getU32(obj, "worleyMode", 0));
      p.worleyJitter = getF(obj, "worleyJitter", p.worleyJitter);
      n->setParams(p);
      newNode = n;

    } else if (type == "PatternGenerator") {
      auto* n = controller->createPatternGeneratorNode(nameStr);
      if (!n) continue;
      node::proc::PatternParams p = n->getParams();
      p.width = getU32(obj, "width", p.width);
      p.height = getU32(obj, "height", p.height);
      p.type = static_cast<node::proc::PatternType>(getU32(obj, "patternType", 0));
      p.scale = getF(obj, "scale", p.scale);
      p.thickness = getF(obj, "thickness", p.thickness);
      p.smoothness = getF(obj, "smoothness", p.smoothness);
      p.param1 = getF(obj, "param1", p.param1);
      p.param2 = getF(obj, "param2", p.param2);
      n->setParams(p);
      newNode = n;

    } else if (type == "Sobel") {
      auto* n = controller->createSobelNode(nameStr);
      if (!n) continue;
      node::op::SobelParams p = n->getParams();
      p.intensity = getF(obj, "intensity", p.intensity);
      p.threshold = getF(obj, "threshold", p.threshold);
      n->setParams(p);
      newNode = n;

    } else if (type == "HeightMap") {
      auto* n = controller->createHeightMapNode(nameStr);
      if (!n) continue;
      node::op::HeightMapParams p = n->getParams();
      p.contrast = getF(obj, "contrast", p.contrast);
      p.brightness = getF(obj, "brightness", p.brightness);
      p.invert = getU32(obj, "invert", p.invert);
      n->setParams(p);
      newNode = n;

    } else if (type == "NormalMap") {
      auto* n = controller->createNormalMapNode(nameStr);
      if (!n) continue;
      node::op::NormalMapParams p = n->getParams();
      p.strength = getF(obj, "strength", p.strength);
      p.invertX = getU32(obj, "invertX", p.invertX);
      p.invertY = getU32(obj, "invertY", p.invertY);
      n->setParams(p);
      newNode = n;

    } else if (type == "Tint") {
      auto* n = controller->createTintNode(nameStr);
      if (!n) continue;
      node::op::TintParams p = n->getParams();
      simdjson::dom::array colorArr;
      if (!obj["color"].get_array().get(colorArr)) {
        int i = 0;
        for (auto el : colorArr) {
          if (i >= 4) break;
          double v; if (!el.get_double().get(v)) p.color[i] = static_cast<float>(v);
          ++i;
        }
      }
      p.factor = getF(obj, "factor", p.factor);
      p.mode = static_cast<node::op::TintMode>(getU32(obj, "tintMode", 0));
      n->setParams(p);
      newNode = n;

    } else if (type == "Mix") {
      auto* n = controller->createMixNode(nameStr);
      if (!n) continue;
      node::op::MixParams p = n->getParams();
      p.width = getU32(obj, "width", p.width);
      p.height = getU32(obj, "height", p.height);
      p.factor = getF(obj, "factor", p.factor);
      p.mode = static_cast<node::op::MixMode>(getU32(obj, "mixMode", 0));
      n->setParams(p);
      newNode = n;

    } else if (type == "Diffuse") {
      auto* n = controller->createDiffuseNode(nameStr);
      if (!n) continue;
      n->alphaMode = static_cast<material::AlphaMode>(getI32(obj, "alphaMode", 0));
      n->diffuseFactor = getVec4(obj, "diffuseFactor", n->diffuseFactor);
      n->markStale();
      newNode = n;

    } else if (type == "DiffuseSpecular") {
      auto* n = controller->createDiffuseSpecularNode(nameStr);
      if (!n) continue;
      n->alphaMode = static_cast<material::AlphaMode>(getI32(obj, "alphaMode", 0));
      n->diffuseFactor = getVec4(obj, "diffuseFactor", n->diffuseFactor);
      n->specularFactor = getVec3(obj, "specularFactor", n->specularFactor);
      n->glossinessFactor = getF(obj, "glossinessFactor", n->glossinessFactor);
      n->markStale();
      newNode = n;

    } else if (type == "PrincipledBSDF") {
      auto* n = controller->createPrincipledBSDFNode(nameStr);
      if (!n) continue;
      n->alphaMode = static_cast<material::AlphaMode>(getI32(obj, "alphaMode", 0));
      n->baseColorFactor = getVec4(obj, "baseColorFactor", n->baseColorFactor);
      n->emissiveFactor = getVec3(obj, "emissiveFactor", n->emissiveFactor);
      n->metallicFactor = getF(obj, "metallicFactor", n->metallicFactor);
      n->roughnessFactor = getF(obj, "roughnessFactor", n->roughnessFactor);
      n->occlusionStrength = getF(obj, "occlusionStrength", n->occlusionStrength);
      n->ior = getF(obj, "ior", n->ior);
      n->specularColorFactor = getVec3(obj, "specularColorFactor", n->specularColorFactor);
      n->specularFactor = getF(obj, "specularFactor", n->specularFactor);
      n->transmissionFactor = getF(obj, "transmissionFactor", n->transmissionFactor);
      n->thicknessFactor = getF(obj, "thicknessFactor", n->thicknessFactor);
      n->attenuationDistance = getF(obj, "attenuationDistance", n->attenuationDistance);
      n->attenuationColor = getVec3(obj, "attenuationColor", n->attenuationColor);
      n->sheenColorFactor = getVec3(obj, "sheenColorFactor", n->sheenColorFactor);
      n->sheenRoughnessFactor = getF(obj, "sheenRoughnessFactor", n->sheenRoughnessFactor);
      n->clearcoatFactor = getF(obj, "clearcoatFactor", n->clearcoatFactor);
      n->clearcoatRoughnessFactor = getF(obj, "clearcoatRoughnessFactor", n->clearcoatRoughnessFactor);
      n->anisotropyStrength = getF(obj, "anisotropyStrength", n->anisotropyStrength);
      n->anisotropyRotation = getVec2(obj, "anisotropyRotation", n->anisotropyRotation);
      n->iridescenceFactor = getF(obj, "iridescenceFactor", n->iridescenceFactor);
      n->iridescenceIor = getF(obj, "iridescenceIor", n->iridescenceIor);
      n->iridescenceThicknessMin = getF(obj, "iridescenceThicknessMin", n->iridescenceThicknessMin);
      n->iridescenceThicknessMax = getF(obj, "iridescenceThicknessMax", n->iridescenceThicknessMax);
      n->markStale();
      newNode = n;

    } else if (type == "ChannelRemap") {
      auto* n = controller->createChannelRemapNode(nameStr);
      if (!n) continue;
      node::op::ChannelRemapParams p = n->getParams();
      p.outR = getU32(obj, "outR", p.outR);
      p.outG = getU32(obj, "outG", p.outG);
      p.outB = getU32(obj, "outB", p.outB);
      p.outA = getU32(obj, "outA", p.outA);
      n->setParams(p);
      newNode = n;

    } else if (type == "ChannelAdjust") {
      auto* n = controller->createChannelAdjustNode(nameStr);
      if (!n) continue;
      node::op::ChannelAdjustParams p = n->getParams();
      p.gain[0] = getF(obj, "gainR", p.gain[0]);
      p.gain[1] = getF(obj, "gainG", p.gain[1]);
      p.gain[2] = getF(obj, "gainB", p.gain[2]);
      p.gain[3] = getF(obj, "gainA", p.gain[3]);
      p.bias[0] = getF(obj, "biasR", p.bias[0]);
      p.bias[1] = getF(obj, "biasG", p.bias[1]);
      p.bias[2] = getF(obj, "biasB", p.bias[2]);
      p.bias[3] = getF(obj, "biasA", p.bias[3]);
      p.invert[0] = getU32(obj, "invertR", p.invert[0] ? 1u : 0u) != 0;
      p.invert[1] = getU32(obj, "invertG", p.invert[1] ? 1u : 0u) != 0;
      p.invert[2] = getU32(obj, "invertB", p.invert[2] ? 1u : 0u) != 0;
      p.invert[3] = getU32(obj, "invertA", p.invert[3] ? 1u : 0u) != 0;
      n->setParams(p);
      newNode = n;

    } else if (type == "MixMaterial") {
      auto* n = controller->createMixMaterialNode(nameStr);
      if (!n) continue;
      n->factor     = getF(obj, "factor",     n->factor);
      n->threshold  = getF(obj, "threshold",  n->threshold);
      n->edge       = getF(obj, "edge",       n->edge);
      n->alphaCutoff = getF(obj, "alphaCutoff", n->alphaCutoff);
      n->alphaMode  = static_cast<material::AlphaMode>(getI32(obj, "alphaMode", static_cast<int32_t>(n->alphaMode)));
      n->markStale();
      newNode = n;

    } else if (type == "SlotOutput") {
      auto* n = controller->createSlotOutputNode(nameStr);
      if (!n) continue;
      n->targetSlotId = getU32(obj, "targetSlotId", n->targetSlotId);
      newNode = n;
    }

    if (newNode) {
      idMap[oldId] = newNode;
      outPositions.push_back(
          {newNode->getId(), static_cast<float>(x), static_cast<float>(y)});
    }
  }

  // Reconnect links
  simdjson::dom::array linksArr;
  if (!doc["links"].get_array().get(linksArr)) {
    for (auto linkEl : linksArr) {
      simdjson::dom::object obj;
      if (linkEl.get_object().get(obj)) continue;

      int64_t srcNodeId = 0, sinkNodeId = 0;
      uint64_t srcSlot = 0, sinkSlot = 0;
      if (obj["srcNodeId"].get_int64().get(srcNodeId)) continue;
      if (obj["srcSlot"].get_uint64().get(srcSlot)) continue;
      if (obj["sinkNodeId"].get_int64().get(sinkNodeId)) continue;
      if (obj["sinkSlot"].get_uint64().get(sinkSlot)) continue;

      auto srcIt  = idMap.find(srcNodeId);
      auto sinkIt = idMap.find(sinkNodeId);
      if (srcIt == idMap.end() || sinkIt == idMap.end()) continue;

      auto& srcOutputs  = srcIt->second->getOutputs();
      auto& sinkInputs  = sinkIt->second->getInputs();
      if (srcSlot >= srcOutputs.size() || sinkSlot >= sinkInputs.size()) continue;

      controller->connectPins(srcOutputs[srcSlot]->getId(),
                              sinkInputs[sinkSlot]->getId());
    }
  }

  if (controller->getWorkflow()) controller->getWorkflow()->markDirty();
  return true;
}

}  // namespace vkit::workflow
