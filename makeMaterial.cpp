#include "ObjRenderer.h"

std::shared_ptr<ShaderData>
ObjRenderer::makeMaterial(const tinyobj::material_t& mat, 
        const std::string& mtl_base_path)
{
    std::shared_ptr<ShaderData> data;
    data.reset(new ShaderDataPhong());
    data->loadData(mat, mtl_base_path);
    return data;
}