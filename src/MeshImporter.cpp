#include "MeshImporter.h"
#include <algorithm>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

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
        return model;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    for (const auto& shape : shapes) {
        ImportedMesh mesh;
        mesh.name = shape.name;

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

    for (cgltf_size i = 0; i < data->meshes_count; ++i) {
        ImportedMesh mesh;
        mesh.name = data->meshes[i].name ? data->meshes[i].name : "Mesh";

        for (cgltf_size j = 0; j < data->meshes[i].primitives_count; ++j) {
            cgltf_primitive* primitive = &data->meshes[i].primitives[j];

            if (primitive->type != cgltf_primitive_type_triangles) continue;

            uint32_t vertexStart = mesh.vertices.size();

            cgltf_accessor* posAccessor = nullptr;
            cgltf_accessor* normAccessor = nullptr;
            cgltf_accessor* uvAccessor = nullptr;

            for (cgltf_size k = 0; k < primitive->attributes_count; ++k) {
                cgltf_attribute* attr = &primitive->attributes[k];
                if (attr->type == cgltf_attribute_type_position) posAccessor = attr->data;
                if (attr->type == cgltf_attribute_type_normal) normAccessor = attr->data;
                if (attr->type == cgltf_attribute_type_texcoord) uvAccessor = attr->data;
            }

            if (!posAccessor) continue;

            for (cgltf_size k = 0; k < posAccessor->count; ++k) {
                MeshVertex vertex;
                vertex.normal = {0.0f, 0.0f, 0.0f};
                vertex.uv = {0.0f, 0.0f};
                cgltf_accessor_read_float(posAccessor, k, &vertex.pos.x, 3);
                if (normAccessor) {
                    cgltf_accessor_read_float(normAccessor, k, &vertex.normal.x, 3);
                    vertex.normal = glm::normalize(vertex.normal);
                }
                if (uvAccessor) {
                    cgltf_accessor_read_float(uvAccessor, k, &vertex.uv.x, 2);
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

        mesh.valid = true;
        model.meshes.push_back(mesh);
    }
    
    cgltf_free(data);
    model.valid = !model.meshes.empty();
    return model;
}

#include "ufbx.h"

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

ImportedModel MeshImporter::Load(const std::string& filePath) {
    try {
        std::filesystem::path path(filePath);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".obj") {
            return LoadOBJ(filePath);
        } else if (ext == ".gltf" || ext == ".glb") {
            return LoadGLTF(filePath);
        } else if (ext == ".fbx") {
            return LoadFBX(filePath);
        }
        
        ImportedModel model;
        model.errorMsg = "Unsupported format";
        return model;
    } catch (const std::exception& ex) {
        ImportedModel model;
        model.errorMsg = ex.what();
        return model;
    } catch (...) {
        ImportedModel model;
        model.errorMsg = "Unknown exception loading model";
        return model;
    }
}
