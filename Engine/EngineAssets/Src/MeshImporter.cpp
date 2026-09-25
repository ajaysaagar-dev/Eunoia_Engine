#include <EngineAssets/MeshImporter.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <mutex>

static std::unordered_map<std::string, ImportedModel> s_modelCache;
static std::mutex s_modelCacheMutex;

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

static void LogShaderResourceValidation(const ImportedMaterial& mat, bool hasTangent, bool hasNormal, bool hasUV) {
    std::cout << "[PBR MATERIAL VALIDATION] Material: " << (mat.name.empty() ? "Material" : mat.name) << "\n"
              << "  BaseColor: " << (mat.baseColorTexture.empty() ? "(None / Factor)" : mat.baseColorTexture) << " [SRGB]\n"
              << "  Normal: " << (mat.normalTexture.empty() ? "(None)" : mat.normalTexture) << " [LINEAR]\n"
              << "  NormalYFlip: " << (mat.normalMapYFlip ? "TRUE (OpenGL/Blender)" : "FALSE (DirectX)") << "\n"
              << "  Metallic: " << (mat.metallicTexture.empty() ? "(None / Factor)" : mat.metallicTexture) << " [LINEAR] (Channel: " << mat.metallicChannel << ")\n"
              << "  Roughness: " << (mat.roughnessTexture.empty() ? "(None / Factor)" : mat.roughnessTexture) << " [LINEAR] (Channel: " << mat.roughnessChannel << ")\n"
              << "  AO: " << (mat.aoTexture.empty() ? "(None / 1.0)" : mat.aoTexture) << " [LINEAR] (Channel: " << mat.aoChannel << ")\n"
              << "  Tangent: " << (hasTangent ? "YES" : "NO") << "\n"
              << "  Vertex normal: " << (hasNormal ? "YES" : "NO") << "\n"
              << "  UV0: " << (hasUV ? "YES" : "NO") << std::endl;
}

PrimitiveMesh ImportedModel::GetMergedMesh() const {
    PrimitiveMesh merged;
    uint32_t offset = 0;
    for (const auto& mesh : meshes) {
        if (!mesh.valid) continue;
        merged.vertices.insert(merged.vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
        for (uint32_t idx : mesh.indices) {
            merged.indices.push_back(idx + offset);
        }
        offset += mesh.vertices.size();
    }
    return merged;
}

ImportedModel MeshImporter::LoadOBJ(const std::string& filePath) {
    ImportedModel model;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filePath, reader_config)) {
        if (!reader.Error().empty()) {
            model.errorMsg = reader.Error();
        }
        std::cout << "[MeshImporter] Notice: Parsing OBJ '" << filePath << "' encountered error: " << model.errorMsg << " (neglecting, force opening).\n";
        return model;
    }

    // Neglect repetitive material-not-found spam and only log non-mtl warnings once
    if (!reader.Warning().empty()) {
        std::istringstream stream(reader.Warning());
        std::string line;
        bool hasLoggedMissingMtl = false;
        while (std::getline(stream, line)) {
            if (line.find("not found in .mtl") != std::string::npos) {
                // Neglect repetitive "not found in .mtl" spam; log a single one-line notice once
                if (!hasLoggedMissingMtl) {
                    hasLoggedMissingMtl = true;
                    std::cout << "[MeshImporter] Notice: Materials not found in .mtl for '" << filePath << "' (neglecting missing materials, using defaults).\n";
                }
            } else if (!line.empty()) {
                std::cout << "[MeshImporter] Warning: " << line << "\n";
            }
        }
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    std::filesystem::path objDir = std::filesystem::path(filePath).parent_path();
    auto resolveObjTex = [&](const std::string& texname) -> std::string {
        if (texname.empty()) return "";
        std::filesystem::path resolved = objDir / texname;
        return std::filesystem::exists(resolved) ? resolved.string() : texname; // bare name lets TextureManager's recursive search try
    };

    for (const auto& shape : shapes) {
        ImportedMesh mesh;
        mesh.name = shape.name;

        if (!shape.mesh.material_ids.empty()) {
            int matId = shape.mesh.material_ids[0];
            if (matId >= 0 && matId < (int)materials.size()) {
                const auto& tm = materials[matId];
                ImportedMaterial& m = mesh.material;
                m.hasMaterial = true;
                m.name = tm.name.empty() ? "Material" : tm.name;
                m.baseColorTexture = resolveObjTex(tm.diffuse_texname);
                m.normalTexture    = resolveObjTex(!tm.normal_texname.empty() ? tm.normal_texname : tm.bump_texname);
                m.roughnessTexture = resolveObjTex(tm.roughness_texname);
                m.metallicTexture  = resolveObjTex(tm.metallic_texname);
                m.aoTexture        = resolveObjTex(tm.ambient_texname);
                m.emissionTexture  = resolveObjTex(tm.emissive_texname);
                m.opacityTexture   = resolveObjTex(tm.alpha_texname);
                if (m.roughnessTexture.empty() && tm.roughness > 0.0f && std::isfinite(tm.roughness)) m.roughness = tm.roughness;
                if (m.metallicTexture.empty() && tm.metallic > 0.0f && std::isfinite(tm.metallic)) m.metallic = tm.metallic;
            }
        }

        for (size_t s = 0; s < shape.mesh.indices.size(); s++) {
            tinyobj::index_t idx = shape.mesh.indices[s];
            MeshVertex vertex;

            vertex.pos = {
                attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                attrib.vertices[3 * size_t(idx.vertex_index) + 2]
            };

            if (idx.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * size_t(idx.normal_index) + 0],
                    attrib.normals[3 * size_t(idx.normal_index) + 1],
                    attrib.normals[3 * size_t(idx.normal_index) + 2]
                };
            } else {
                vertex.normal = {0.0f, 0.0f, 0.0f};
            }

            if (idx.texcoord_index >= 0) {
                vertex.uv = {
                    attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                    attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]
                };
            } else {
                vertex.uv = {0.0f, 0.0f};
            }

            mesh.vertices.push_back(vertex);
            mesh.indices.push_back((uint32_t)mesh.vertices.size() - 1);
        }

        // Compute flat normals if missing
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            MeshVertex& v0 = mesh.vertices[mesh.indices[i]];
            MeshVertex& v1 = mesh.vertices[mesh.indices[i+1]];
            MeshVertex& v2 = mesh.vertices[mesh.indices[i+2]];

            if (glm::length(v0.normal) < 0.001f || glm::length(v1.normal) < 0.001f || glm::length(v2.normal) < 0.001f) {
                glm::vec3 normal = glm::normalize(glm::cross(v1.pos - v0.pos, v2.pos - v0.pos));
                v0.normal = normal;
                v1.normal = normal;
                v2.normal = normal;
            }
        }

        GeometryBuilder::CalculateTangents(mesh.vertices, mesh.indices);
        LogShaderResourceValidation(mesh.material, true, true, !attrib.texcoords.empty());

        mesh.valid = true;
        model.meshes.push_back(mesh);
    }
    model.valid = !model.meshes.empty();
    return model;
}

ImportedModel MeshImporter::LoadGLTF(const std::string& filePath) {
    ImportedModel model;
    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, filePath.c_str(), &data);

    if (result != cgltf_result_success) {
        model.errorMsg = "Failed to parse GLTF file";
        return model;
    }

    result = cgltf_load_buffers(&options, data, filePath.c_str());
    if (result != cgltf_result_success) {
        model.errorMsg = "Failed to load GLTF buffers";
        cgltf_free(data);
        return model;
    }

    result = cgltf_validate(data);
    if (result != cgltf_result_success) {
        model.errorMsg = "Failed to validate GLTF";
        cgltf_free(data);
        return model;
    }

    std::filesystem::path gltfDir = std::filesystem::path(filePath).parent_path();
    std::filesystem::path gltfCacheDir = gltfDir / "ImportedTextures";
    auto resolveGltfImage = [&](cgltf_texture* tex, const std::string& matName, const std::string& slotName) -> std::string {
        if (!tex || !tex->image) return "";
        cgltf_image* img = tex->image;
        if (img->uri) {
            std::string uri = img->uri;
            if (uri.rfind("data:", 0) == 0) {
                // Embedded base64 data URI — cgltf can decode this for us via buffer_view if present;
                // fall through to buffer_view handling below if uri decoding isn't wired up.
            } else {
                std::filesystem::path resolved = gltfDir / uri;
                if (std::filesystem::exists(resolved)) return resolved.string();
            }
        }
        if (img->buffer_view) {
            // GLB-embedded or data-URI image: extract raw bytes and cache to disk.
            std::error_code ec;
            std::filesystem::create_directories(gltfCacheDir, ec);
            const uint8_t* dataBytes = (const uint8_t*)cgltf_buffer_view_data(img->buffer_view);
            size_t size = img->buffer_view->size;
            if (dataBytes && size > 0) {
                std::string ext = ".png";
                if (img->mime_type && std::string(img->mime_type).find("jpeg") != std::string::npos) ext = ".jpg";
                std::filesystem::path outPath = gltfCacheDir / (matName + "_" + slotName + ext);
                FILE* f = fopen(outPath.string().c_str(), "wb");
                if (f) { fwrite(dataBytes, 1, size, f); fclose(f); return outPath.string(); }
            }
        }
        return "";
    };

    for (cgltf_size i = 0; i < data->meshes_count; ++i) {
        ImportedMesh mesh;
        mesh.name = data->meshes[i].name ? data->meshes[i].name : "Mesh";

        for (cgltf_size j = 0; j < data->meshes[i].primitives_count; ++j) {
            cgltf_primitive* primitive = &data->meshes[i].primitives[j];

            if (primitive->type != cgltf_primitive_type_triangles) continue;

            if (!mesh.material.hasMaterial && primitive->material) {
                cgltf_material* mat = primitive->material;
                ImportedMaterial& m = mesh.material;
                m.hasMaterial = true;
                m.name = mat->name ? mat->name : "Material";
                if (mat->has_pbr_metallic_roughness) {
                    auto& pbr = mat->pbr_metallic_roughness;
                    m.baseColorTexture = resolveGltfImage(pbr.base_color_texture.texture, m.name, "BaseColor");
                    m.metallicTexture  = resolveGltfImage(pbr.metallic_roughness_texture.texture, m.name, "MetallicRoughness");
                    m.roughnessTexture = m.metallicTexture; // packed GLTF metallic-roughness
                    // GLTF specification: Green channel = Roughness, Blue channel = Metallic
                    m.roughnessChannel = 1;
                    m.metallicChannel = 2;
                    m.aoChannel = 0;
                    if (std::isfinite(pbr.base_color_factor[0])) {
                        m.baseColor = glm::vec3(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]);
                    }
                    if (m.metallicTexture.empty() && std::isfinite(pbr.metallic_factor)) m.metallic = pbr.metallic_factor;
                    if (m.roughnessTexture.empty() && std::isfinite(pbr.roughness_factor)) m.roughness = pbr.roughness_factor;
                }
                m.normalTexture   = resolveGltfImage(mat->normal_texture.texture, m.name, "Normal");
                m.normalMapYFlip  = true; // GLTF standard expects OpenGL normal map convention
                m.aoTexture       = resolveGltfImage(mat->occlusion_texture.texture, m.name, "AO");
                m.emissionTexture = resolveGltfImage(mat->emissive_texture.texture, m.name, "Emissive");
                if (std::isfinite(mat->emissive_factor[0])) {
                    m.emissiveColor = glm::vec3(mat->emissive_factor[0], mat->emissive_factor[1], mat->emissive_factor[2]);
                }
                if (mat->alpha_mode == cgltf_alpha_mode_mask) {
                    m.blendMode = 1;
                    m.opacityMaskClipValue = std::isfinite(mat->alpha_cutoff) ? mat->alpha_cutoff : 0.5f;
                } else if (mat->alpha_mode == cgltf_alpha_mode_blend) {
                    m.blendMode = 2;
                } else {
                    m.blendMode = 0;
                }
            }

            uint32_t vertexStart = mesh.vertices.size();

            cgltf_accessor* posAccessor = nullptr;
            cgltf_accessor* normAccessor = nullptr;
            cgltf_accessor* uvAccessor = nullptr;
            cgltf_accessor* tanAccessor = nullptr;

            for (cgltf_size k = 0; k < primitive->attributes_count; ++k) {
                cgltf_attribute* attr = &primitive->attributes[k];
                if (attr->type == cgltf_attribute_type_position) posAccessor = attr->data;
                if (attr->type == cgltf_attribute_type_normal) normAccessor = attr->data;
                if (attr->type == cgltf_attribute_type_texcoord) uvAccessor = attr->data;
                if (attr->type == cgltf_attribute_type_tangent) tanAccessor = attr->data;
            }

            if (!posAccessor) continue;

            for (cgltf_size k = 0; k < posAccessor->count; ++k) {
                MeshVertex vertex;
                vertex.normal = {0.0f, 0.0f, 0.0f};
                vertex.uv = {0.0f, 0.0f};
                vertex.tangent = {1.0f, 0.0f, 0.0f, 1.0f};
                cgltf_accessor_read_float(posAccessor, k, &vertex.pos.x, 3);
                if (normAccessor) {
                    cgltf_accessor_read_float(normAccessor, k, &vertex.normal.x, 3);
                    vertex.normal = glm::normalize(vertex.normal);
                }
                if (uvAccessor) {
                    cgltf_accessor_read_float(uvAccessor, k, &vertex.uv.x, 2);
                }
                if (tanAccessor) {
                    cgltf_accessor_read_float(tanAccessor, k, &vertex.tangent.x, 4);
                }
                mesh.vertices.push_back(vertex);
            }

            if (primitive->indices) {
                for (cgltf_size k = 0; k < primitive->indices->count; ++k) {
                    mesh.indices.push_back(vertexStart + cgltf_accessor_read_index(primitive->indices, k));
                }
            } else {
                for (cgltf_size k = 0; k < posAccessor->count; ++k) {
                    mesh.indices.push_back(vertexStart + k);
                }
            }
        }
        
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            MeshVertex& v0 = mesh.vertices[mesh.indices[i]];
            MeshVertex& v1 = mesh.vertices[mesh.indices[i+1]];
            MeshVertex& v2 = mesh.vertices[mesh.indices[i+2]];

            if (glm::length(v0.normal) < 0.001f || glm::length(v1.normal) < 0.001f || glm::length(v2.normal) < 0.001f) {
                glm::vec3 normal = glm::normalize(glm::cross(v1.pos - v0.pos, v2.pos - v0.pos));
                v0.normal = normal;
                v1.normal = normal;
                v2.normal = normal;
            }
        }

        bool hasImportedTangents = false;
        for (const auto& v : mesh.vertices) {
            if (v.tangent != glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)) {
                hasImportedTangents = true;
                break;
            }
        }
        if (!hasImportedTangents) {
            GeometryBuilder::CalculateTangents(mesh.vertices, mesh.indices);
        }

        LogShaderResourceValidation(mesh.material, true, true, true);

        mesh.valid = true;
        model.meshes.push_back(mesh);
    }
    
    cgltf_free(data);
    model.valid = !model.meshes.empty();
    return model;
}

#include "ufbx.h"

static std::string ResolveFbxTexturePath(ufbx_texture* tex, const std::filesystem::path& fbxDir,
                                          const std::filesystem::path& cacheDir, const std::string& matName,
                                          const std::string& slotName) {
    if (!tex) return "";

    // Embedded media: extract to a cache file next to the source FBX so the
    // existing path-based TextureManager can load it with no renderer changes.
    if (tex->content.size > 0) {
        std::error_code ec;
        std::filesystem::create_directories(cacheDir, ec);
        std::string ext = ".png";
        if (tex->filename.length > 0) {
            std::string origExt = std::filesystem::path(std::string(tex->filename.data, tex->filename.length)).extension().string();
            if (!origExt.empty()) ext = origExt;
        }
        std::filesystem::path outPath = cacheDir / (matName + "_" + slotName + ext);
        FILE* f = fopen(outPath.string().c_str(), "wb");
        if (f) {
            fwrite(tex->content.data, 1, tex->content.size, f);
            fclose(f);
            return outPath.string();
        }
        return "";
    }

    // External reference: prefer absolute filename, fall back to relative-to-FBX-dir.
    if (tex->filename.length > 0) {
        std::string abs(tex->filename.data, tex->filename.length);
        if (std::filesystem::exists(abs)) return abs;
    }
    if (tex->relative_filename.length > 0) {
        std::filesystem::path rel(std::string(tex->relative_filename.data, tex->relative_filename.length));
        std::filesystem::path resolved = fbxDir / rel;
        if (std::filesystem::exists(resolved)) return resolved.string();
        return resolved.string(); // let TextureManager's recursive search try the bare filename
    }
    return "";
}

ImportedModel MeshImporter::LoadFBX(const std::string& filePath) {
    ImportedModel model;
    ufbx_load_opts opts = { 0 };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;
    opts.generate_missing_normals = true;

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(filePath.c_str(), &opts, &error);
    if (!scene) {
        char errBuf[512];
        ufbx_format_error(errBuf, sizeof(errBuf), &error);
        model.errorMsg = errBuf;
        return model;
    }

    std::vector<uint32_t> tri_indices;
    std::filesystem::path fbxDir = std::filesystem::path(filePath).parent_path();
    std::filesystem::path cacheDir = fbxDir / "ImportedTextures";

    for (size_t i = 0; i < scene->meshes.count; ++i) {
        ufbx_mesh* mesh = scene->meshes.data[i];
        if (!mesh || mesh->num_faces == 0) continue;

        ImportedMesh im;
        if (mesh->name.data && mesh->name.length > 0) {
            im.name = std::string(mesh->name.data, mesh->name.length);
        } else if (mesh->instances.count > 0 && mesh->instances.data[0]->name.data) {
            im.name = std::string(mesh->instances.data[0]->name.data, mesh->instances.data[0]->name.length);
        } else {
            im.name = "FBX_Mesh_" + std::to_string(i);
        }

        size_t max_tris = mesh->max_face_triangles * 3;
        if (max_tris > tri_indices.size()) {
            tri_indices.resize(std::max(max_tris, (size_t)64));
        }

        for (size_t f = 0; f < mesh->faces.count; ++f) {
            ufbx_face face = mesh->faces.data[f];
            uint32_t num_tris = ufbx_triangulate_face(tri_indices.data(), tri_indices.size(), mesh, face);

            for (uint32_t t = 0; t < num_tris * 3; ++t) {
                uint32_t idx = tri_indices[t];
                MeshVertex v;
                ufbx_vec3 p = ufbx_get_vertex_vec3(&mesh->vertex_position, idx);
                v.pos = { (float)p.x, (float)p.y, (float)p.z };

                if (mesh->vertex_normal.exists) {
                    ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->vertex_normal, idx);
                    v.normal = { (float)n.x, (float)n.y, (float)n.z };
                } else {
                    v.normal = { 0.0f, 1.0f, 0.0f };
                }

                if (mesh->vertex_uv.exists) {
                    ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, idx);
                    v.uv = { (float)uv.x, (float)uv.y };
                } else {
                    v.uv = { 0.0f, 0.0f };
                }

                if (mesh->vertex_tangent.exists) {
                    ufbx_vec3 t = ufbx_get_vertex_vec3(&mesh->vertex_tangent, idx);
                    ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->vertex_normal, idx);
                    float h = 1.0f;
                    if (mesh->vertex_bitangent.exists) {
                        ufbx_vec3 b = ufbx_get_vertex_vec3(&mesh->vertex_bitangent, idx);
                        glm::vec3 gn((float)n.x, (float)n.y, (float)n.z);
                        glm::vec3 gt((float)t.x, (float)t.y, (float)t.z);
                        glm::vec3 gb((float)b.x, (float)b.y, (float)b.z);
                        h = (glm::dot(glm::cross(gn, gt), gb) < 0.0f) ? -1.0f : 1.0f;
                    }
                    v.tangent = { (float)t.x, (float)t.y, (float)t.z, h };
                }

                im.vertices.push_back(v);
                im.indices.push_back((uint32_t)im.vertices.size() - 1);
            }
        }

        for (size_t k = 0; k + 2 < im.indices.size(); k += 3) {
            MeshVertex& v0 = im.vertices[im.indices[k]];
            MeshVertex& v1 = im.vertices[im.indices[k+1]];
            MeshVertex& v2 = im.vertices[im.indices[k+2]];
            if (glm::length(v0.normal) < 0.001f || glm::length(v1.normal) < 0.001f || glm::length(v2.normal) < 0.001f) {
                glm::vec3 n = glm::normalize(glm::cross(v1.pos - v0.pos, v2.pos - v0.pos));
                v0.normal = n;
                v1.normal = n;
                v2.normal = n;
            }
        }

        if (!mesh->vertex_tangent.exists) {
            GeometryBuilder::CalculateTangents(im.vertices, im.indices);
        }

        if (mesh->materials.count > 0 && mesh->materials.data[0]) {
            ufbx_material* mat = mesh->materials.data[0];
            ImportedMaterial& m = im.material;
            m.hasMaterial = true;
            m.name = mat->name.length > 0 ? std::string(mat->name.data, mat->name.length) : "Material";

            m.baseColorTexture = ResolveFbxTexturePath(mat->pbr.base_color.texture, fbxDir, cacheDir, m.name, "BaseColor");
            m.normalTexture    = ResolveFbxTexturePath(mat->pbr.normal_map.texture, fbxDir, cacheDir, m.name, "Normal");
            m.roughnessTexture = ResolveFbxTexturePath(mat->pbr.roughness.texture, fbxDir, cacheDir, m.name, "Roughness");
            m.metallicTexture  = ResolveFbxTexturePath(mat->pbr.metalness.texture, fbxDir, cacheDir, m.name, "Metallic");
            m.aoTexture        = ResolveFbxTexturePath(mat->pbr.ambient_occlusion.texture, fbxDir, cacheDir, m.name, "AO");
            m.emissionTexture  = ResolveFbxTexturePath(mat->pbr.emission_color.texture, fbxDir, cacheDir, m.name, "Emissive");
            m.normalMapYFlip   = true; // Blender FBX exports normal maps in OpenGL standard convention

            if (mat->pbr.base_color.has_value) {
                ufbx_vec3 bc = mat->pbr.base_color.value_vec3;
                if (std::isfinite((float)bc.x) && std::isfinite((float)bc.y) && std::isfinite((float)bc.z))
                    m.baseColor = glm::vec3((float)bc.x, (float)bc.y, (float)bc.z);
            }
            if (mat->pbr.opacity.texture) {
                m.opacityTexture = ResolveFbxTexturePath(mat->pbr.opacity.texture, fbxDir, cacheDir, m.name, "Opacity");
            }
            if (mat->pbr.opacity.has_value && std::isfinite((float)mat->pbr.opacity.value_real)) {
                m.opacity = (float)mat->pbr.opacity.value_real;
            }
            if (!m.opacityTexture.empty() || m.opacity < 0.999f) {
                m.blendMode = 1;
            }

            // Constant fallbacks when a slot has a value but no texture (e.g. a flat metalness/roughness number)
            if (mat->pbr.metalness.has_value && m.metallicTexture.empty())
                m.metallic = std::isfinite((float)mat->pbr.metalness.value_real) ? (float)mat->pbr.metalness.value_real : 0.0f;
            if (mat->pbr.roughness.has_value && m.roughnessTexture.empty())
                m.roughness = std::isfinite((float)mat->pbr.roughness.value_real) ? (float)mat->pbr.roughness.value_real : 0.5f;
            if (mat->pbr.emission_color.has_value) {
                ufbx_vec3 ec = mat->pbr.emission_color.value_vec3;
                if (std::isfinite((float)ec.x) && std::isfinite((float)ec.y) && std::isfinite((float)ec.z))
                    m.emissiveColor = glm::vec3((float)ec.x, (float)ec.y, (float)ec.z);
            }
            if (mat->pbr.emission_factor.has_value && std::isfinite((float)mat->pbr.emission_factor.value_real))
                m.emissiveIntensity = (float)mat->pbr.emission_factor.value_real;
        }

        LogShaderResourceValidation(im.material, true, mesh->vertex_normal.exists, mesh->vertex_uv.exists);

        im.valid = !im.vertices.empty();
        if (im.valid) {
            model.meshes.push_back(im);
        }
    }

    ufbx_free_scene(scene);
    model.valid = !model.meshes.empty();
    return model;
}

bool MeshImporter::IsSupportedFormat(const std::string& ext) {
    std::string lowerExt = ext;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
    return lowerExt == ".obj" || lowerExt == ".gltf" || lowerExt == ".glb" || lowerExt == ".fbx";
}

void MeshImporter::ClearCache() {
    std::lock_guard<std::mutex> lock(s_modelCacheMutex);
    s_modelCache.clear();
}

void MeshImporter::InvalidateCache(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(s_modelCacheMutex);
    s_modelCache.erase(filePath);
}

ImportedModel MeshImporter::Load(const std::string& filePath) {
    if (filePath.empty()) {
        ImportedModel m;
        m.errorMsg = "Empty file path";
        return m;
    }

    // 1. Check in-memory cache first to avoid re-parsing heavy models repeatedly
    {
        std::lock_guard<std::mutex> lock(s_modelCacheMutex);
        auto it = s_modelCache.find(filePath);
        if (it != s_modelCache.end() && it->second.valid) {
            return it->second;
        }
    }

    try {
        std::filesystem::path path(filePath);
        if (!std::filesystem::exists(path)) {
            ImportedModel model;
            model.errorMsg = "File does not exist: " + filePath;
            std::cout << "[MeshImporter] Notice: " << model.errorMsg << " (neglecting, force opening scene).\n";
            return model;
        }

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        ImportedModel model;
        if (ext == ".obj") {
            model = LoadOBJ(filePath);
        } else if (ext == ".gltf" || ext == ".glb") {
            model = LoadGLTF(filePath);
        } else if (ext == ".fbx") {
            model = LoadFBX(filePath);
        } else {
            model.errorMsg = "Unsupported format: " + ext;
            return model;
        }

        // Cache successfully loaded models
        if (model.valid) {
            std::lock_guard<std::mutex> lock(s_modelCacheMutex);
            s_modelCache[filePath] = model;
        }
        
        return model;
    } catch (const std::exception& ex) {
        ImportedModel model;
        model.errorMsg = ex.what();
        std::cout << "[MeshImporter] Exception loading '" << filePath << "': " << ex.what() << " (neglecting, force opening).\n";
        return model;
    } catch (...) {
        ImportedModel model;
        model.errorMsg = "Unknown exception loading model";
        std::cout << "[MeshImporter] Unknown exception loading '" << filePath << "' (neglecting, force opening).\n";
        return model;
    }
}
