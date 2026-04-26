#ifndef TYPES_GLSL
#define TYPES_GLSL

struct Attribute {
  uvec2 addr;
  uint count;
  uint offset;
  uint stride;
  uint format;
};

struct Primitive {
  Attribute position;
  Attribute color;
  Attribute normal;
  Attribute texcoords[4];
  Attribute tangent;
  Attribute bitangent;
  Attribute jointIndices;
  Attribute jointWeights;
};

bool isAttributeExists(Attribute attr)
{
  return (attr.addr.x != 0u || attr.addr.y != 0u);
}

#endif // TYPES_GLSL
