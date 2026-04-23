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

bool isAttributeExisting(Attribute attr)
{
    return (attr.addr.x != 0u || attr.addr.y != 0u);
}

uvec2 add64(uvec2 a, uint b)
{
    uint lo = a.x + b;
    uint carry = (lo < a.x) ? 1u : 0u;
    return uvec2(lo, a.y + carry);
}

uvec2 add64(uvec2 a, uvec2 b)
{
    uint lo = a.x + b.x;
    uint carry = (lo < a.x) ? 1u : 0u;
    return uvec2(lo, a.y + b.y + carry);
}

uvec2 getFetchAddress(Attribute attr, uint vertexIndex)
{
    uint byteOffset = attr.offset + attr.stride * vertexIndex;
    return add64(attr.addr, byteOffset);
}

#endif // TYPES_GLSL
