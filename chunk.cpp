#include "chunk.h"

void Chunk::build_basics(const Ref<PresetsData>& presets_data, const Ref<VoxelRoomData>& voxel_room_data, Ref<BasicsPreset>& basics_preset, const Vector3i& local_position, const Vector3i& rotation) {

#define BUILD_MESH_NO_CONDITION(direction)\
	do{\
		int material_id = basics_preset->get_##direction();\
		ERR_FAIL_INDEX(material_id, mesh_arrays.size());\
		Array arrays = mesh_arrays[material_id];\
		BasicsPreset::build_##direction##_mesh(arrays, local_position, rotation);\
	} while (false)

#define BUILD_MESH(direction,axis)\
	do{\
		Voxel direction##voxel = get_voxel(local_position + VoxelRoom::get_voxel_direction(axis, rotation));\
		int direction##voxel_type = VoxelRoom::get_voxel_type(direction##voxel);\
		int direction##voxel_id = VoxelRoom::get_voxel_id(direction##voxel);\
		if (direction##voxel_type != VoxelRoomData::BASICS) {\
			int material_id = basics_preset->get_##direction();\
			ERR_FAIL_INDEX(material_id, mesh_arrays.size());\
			Array arrays = mesh_arrays[material_id];\
			BasicsPreset::build_##direction####_mesh(arrays, local_position, rotation);\
		}else{\
			Ref<BasicsPreset> direction##preset = basics_presets[direction##voxel_id];\
			if(direction##preset->get_transparent() != basics_preset->get_transparent()){\
				int material_id = basics_preset->get_##direction();\
				ERR_FAIL_INDEX(material_id, mesh_arrays.size());\
				Array arrays = mesh_arrays[material_id];\
				BasicsPreset::build_##direction####_mesh(arrays, local_position, rotation);\
			}\
		}\
	}while(false)

	Array basics_presets = presets_data->get_basics_presets();
	if (rotation.x % 90 != 0 && rotation.y % 90 != 0 && rotation.z % 90 != 0)
	{
		BUILD_MESH_NO_CONDITION(up);
		BUILD_MESH_NO_CONDITION(down);
		BUILD_MESH_NO_CONDITION(front);
		BUILD_MESH_NO_CONDITION(back);
		BUILD_MESH_NO_CONDITION(left);
		BUILD_MESH_NO_CONDITION(right);
	}
	else {
		BUILD_MESH(up, Vector3i(0, 1, 0));
		BUILD_MESH(down, Vector3i(0, -1, 0));
		BUILD_MESH(front, Vector3i(0, 0, -1));
		BUILD_MESH(back, Vector3i(0, 0, 1));
		BUILD_MESH(left, Vector3i(-1, 0, 0));
		BUILD_MESH(right, Vector3i(1, 0, 0));
	}
#undef BUILD_MESH_NO_CONDITION
#undef BUILD_MESH
}

void Chunk::build_mesh(const Ref<VoxelRoomData>& voxel_room_data, Ref<MeshPreset>& mesh_preset, const Vector3i& local_position, const Vector3i& rotation)
{
	Ref<Mesh> mesh = mesh_preset->get_mesh();
	ERR_FAIL_NULL(mesh);
	TypedArray<int> materials = mesh_preset->get_materials();
	for (int i = 0; i < mesh->get_surface_count(); i++)
	{
		int material_id = materials[i];
		ERR_FAIL_INDEX(material_id, mesh_arrays.size());
		Array arrays = mesh_arrays[material_id];
		mesh_preset->build_mesh(arrays, i, local_position, rotation);
	}
}

void Chunk::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_chunk_position", "value"), &Chunk::set_chunk_position);
	ClassDB::bind_method(D_METHOD("get_chunk_position"), &Chunk::get_chunk_position);

	ClassDB::bind_method(D_METHOD("set_voxel", "local_position", "value"), &Chunk::set_voxel);
	ClassDB::bind_method(D_METHOD("get_voxel", "local_position"), &Chunk::get_voxel);

	ClassDB::bind_method(D_METHOD("generate_mesh", "filter"), &Chunk::generate_mesh, BASICS_FILTER | MESH_FILTER);
	ClassDB::bind_method(D_METHOD("generate_collider", "filter"), &Chunk::generate_collider, BASICS_FILTER | MESH_FILTER);
	ClassDB::bind_method(D_METHOD("generate_device"), &Chunk::generate_device);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "chunk_position"), "set_chunk_position", "get_chunk_position");

	BIND_ENUM_CONSTANT(EMPTY_FILTER);
	BIND_ENUM_CONSTANT(BASICS_FILTER);
	BIND_ENUM_CONSTANT(MESH_FILTER);
	BIND_ENUM_CONSTANT(DEVICE_FILTER);
	BIND_ENUM_CONSTANT(TRANSPARENT_TRUE_FILTER);
	BIND_ENUM_CONSTANT(TRANSPARENT_FALSE_FILTER);
	BIND_ENUM_CONSTANT(COLLIDER_TRUE_FILTER);
	BIND_ENUM_CONSTANT(COLLIDER_FALSE_FILTER);
}

Chunk::Chunk() {
	this->chunk_position = Vector3i(0, 0, 0);
}

Chunk::~Chunk() {

}

VoxelRoom* Chunk::get_voxel_room() const
{
	return cast_to<VoxelRoom>(get_parent());
}

void Chunk::set_chunk_position(const Vector3i& value) {
	ERR_FAIL_COND_MSG(value.x < 0 || value.y < 0 || value.z < 0, "Chunk position is an invalid value");
	chunk_position = value;
}

Vector3i Chunk::get_chunk_position() const {
	return chunk_position;
}


Dictionary Chunk::get_devices() const
{
	return devices;
}

/// <summary>
/// 设置体素信息
/// </summary>
/// <param name="local_position">本地坐标</param>
/// <param name="value"></param>
void Chunk::set_voxel(const Vector3i& local_position, const Voxel& value)
{
	VoxelRoom* voxel_room = get_voxel_room();
	ERR_FAIL_NULL_MSG(voxel_room, "Chunk node must be a direct child of VoxelRoom");
	Ref<VoxelRoomData> voxel_room_data = voxel_room->get_voxel_room_data();
	ERR_FAIL_NULL(voxel_room_data);

	Vector3i chunk_size = voxel_room_data->get_chunk_size();
	if (voxel_room->get_isolated() == true) {
		if (local_position.x < 0 || local_position.x >= chunk_size.x ||
			local_position.y < 0 || local_position.y >= chunk_size.y ||
			local_position.z < 0 || local_position.z >= chunk_size.z) {
			return;
		}
	}
	voxel_room->set_voxel(local_position + chunk_position * voxel_room_data->get_chunk_size(), value);
}

/// <summary>
/// 获取体素信息(在本地坐标中)
/// </summary>
/// <param name="local_position">本地坐标</param>
/// <returns></returns>
Voxel Chunk::get_voxel(const Vector3i& local_position) const
{
	VoxelRoom* voxel_room = get_voxel_room();
	ERR_FAIL_NULL_V_MSG(voxel_room, EMPTY_VOXEL, "Chunk node must be a direct child of VoxelRoom");
	Ref<VoxelRoomData> voxel_room_data = voxel_room->get_voxel_room_data();
	ERR_FAIL_NULL_V(voxel_room_data, EMPTY_VOXEL);

	Vector3i chunk_size = voxel_room_data->get_chunk_size();
	if (voxel_room->get_isolated() == true) {
		if (local_position.x < 0 || local_position.x >= chunk_size.x ||
			local_position.y < 0 || local_position.y >= chunk_size.y ||
			local_position.z < 0 || local_position.z >= chunk_size.z) {
			return EMPTY_VOXEL;
		}
	}
	return voxel_room->get_voxel(local_position + chunk_position * voxel_room_data->get_chunk_size());
}

Vector3i Chunk::get_voxel_local_position(const Vector3& point, const Vector3& normal) const
{
	Vector3 voxel_position = point - get_position();
	if (normal.y == 1) {
		voxel_position.y -= 0.05;
	}
	else if (normal.y == -1) {
		voxel_position.y += 0.05;
	}
	else if (normal.x == 1) {
		voxel_position.x -= 0.05;
	}
	else if (normal.x == -1) {
		voxel_position.x += 0.05;
	}
	else if (normal.z == 1) {
		voxel_position.z -= 0.05;
	}
	else if (normal.z == -1) {
		voxel_position.z += 0.05;
	}
	return Vector3i(voxel_position.round());
}

/// <summary>
/// 生成区块内所有可渲染网格
/// </summary>
/// <returns></returns>
Ref<ArrayMesh> Chunk::generate_mesh(const int& filter)
{
	VoxelRoom* voxel_room = get_voxel_room();
	ERR_FAIL_NULL_V_MSG(voxel_room, nullptr, "Chunk node must be a direct child of VoxelRoom");

	Ref<VoxelRoomData> voxel_room_data = voxel_room->get_voxel_room_data();
	ERR_FAIL_NULL_V(voxel_room_data, nullptr);

	Ref<PresetsData> presets_data = voxel_room->get_presets_data();
	ERR_FAIL_NULL_V(presets_data, nullptr);


	bool is_generate_basics = (filter & BASICS_FILTER) != 0;
	bool is_generate_mesh = (filter & MESH_FILTER) != 0;

	bool is_generate_transparent_true = (filter & TRANSPARENT_TRUE_FILTER) != 0;
	bool is_generate_transparent_false = (filter & TRANSPARENT_FALSE_FILTER) != 0;
	bool is_generate_transparent_all = is_generate_transparent_true == is_generate_transparent_false;

	bool is_generate_collider_true = (filter & COLLIDER_TRUE_FILTER) != 0;
	bool is_generate_collider_false = (filter & COLLIDER_FALSE_FILTER) != 0;
	bool is_generate_collider_all = is_generate_collider_true == is_generate_collider_false;


	Array custom_materials = presets_data->get_custom_materials();
	Array basics_presets = presets_data->get_basics_presets();
	Array mesh_presets = presets_data->get_mesh_presets();

	mesh_arrays.resize(custom_materials.size());
	for (int i = 0; i < mesh_arrays.size(); i++)
	{
		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);
		arrays[Mesh::ARRAY_VERTEX] = Array();
		arrays[Mesh::ARRAY_TEX_UV] = Array();
		arrays[Mesh::ARRAY_NORMAL] = Array();
		mesh_arrays[i] = arrays;
	}

	Vector3i chunk_size = voxel_room_data->get_chunk_size();
	for (int x = 0; x < chunk_size.x; x++)
	{
		for (int y = 0; y < chunk_size.y; y++)
		{
			for (int z = 0; z < chunk_size.z; z++)
			{
				Vector3i local_position = Vector3i(x, y, z);
				Voxel voxel = get_voxel(local_position);
				int type = VoxelRoom::get_voxel_type(voxel);
				int id = VoxelRoom::get_voxel_id(voxel);
				int flag = VoxelRoom::get_voxel_flag(voxel);
				switch (type)
				{
				case VoxelRoomData::BASICS: {
					Vector3i rotation = VoxelRoom::get_voxel_rotation(voxel);

					ERR_FAIL_INDEX_V(id, basics_presets.size(), nullptr);
					Ref<BasicsPreset> basics_preset = basics_presets[id];
					ERR_FAIL_NULL_V_MSG(basics_preset, nullptr, "The basics_preset with id " + String::num_int64(id) + " is null");

					if (is_generate_basics == false)
					{
						continue;
					}
					if (is_generate_transparent_all == false)
					{
						if (is_generate_transparent_true == true && basics_preset->get_transparent() == true)
						{
							continue;
						}
						if (is_generate_transparent_false == true && basics_preset->get_transparent() == false)
						{
							continue;
						}
					}
					if (is_generate_collider_all == false)
					{
						if (is_generate_collider_true == true && basics_preset->get_collider() == true)
						{
							continue;
						}
						if (is_generate_collider_false == true && basics_preset->get_collider() == false)
						{
							continue;
						}
					}
					build_basics(presets_data, voxel_room_data, basics_preset, local_position, rotation);
					break;
				}
				case VoxelRoomData::MESH: {

					ERR_FAIL_INDEX_V(id, mesh_presets.size(), nullptr);
					Ref<MeshPreset> mesh_preset = mesh_presets[id];
					ERR_FAIL_NULL_V_MSG(mesh_preset, nullptr, "The mesh_preset with id " + String::num_int64(id) + " is null");

					if (is_generate_mesh == false)
					{
						continue;
					}
					if (is_generate_transparent_all == false)
					{
						if (is_generate_transparent_true == true && mesh_preset->get_transparent() == true)
						{
							continue;
						}
						if (is_generate_transparent_false == true && mesh_preset->get_transparent() == false)
						{
							continue;
						}
					}
					if (is_generate_collider_all == false)
					{
						if (is_generate_collider_true == true && mesh_preset->get_collider() == true)
						{
							continue;
						}
						if (is_generate_collider_false == true && mesh_preset->get_collider() == false)
						{
							continue;
						}
					}
					Vector3i rotation = VoxelRoom::get_voxel_rotation(voxel);
					build_mesh(voxel_room_data, mesh_preset, local_position, rotation);
					break;
				}
				}
			}
		}
	}
	Ref<ArrayMesh> result;
	result.instantiate();
	for (int i = 0; i < mesh_arrays.size(); i++)
	{
		if (mesh_arrays[i].get_type() != Variant::NIL) {
			Array arrays = mesh_arrays[i];
			arrays[ArrayMesh::ARRAY_VERTEX] = PackedVector3Array((Array)arrays[ArrayMesh::ARRAY_VERTEX]);
			arrays[ArrayMesh::ARRAY_NORMAL] = PackedVector3Array((Array)arrays[ArrayMesh::ARRAY_NORMAL]);
			arrays[ArrayMesh::ARRAY_TEX_UV] = PackedVector2Array((Array)arrays[ArrayMesh::ARRAY_TEX_UV]);
			Ref<ShaderMaterial> material = cast_to<CustomMaterial>(custom_materials[i])->get_shader_material();
			result->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
			result->surface_set_material(result->get_surface_count() - 1, material);
		}
	}
	return result;
}
/// <summary>
/// 将每个体素视作正方体，以生成区块的碰撞网格
/// </summary>
/// <returns></returns>
Ref<ConcavePolygonShape3D> Chunk::generate_collider(const int& filter)
{
	VoxelRoom* voxel_room = get_voxel_room();
	ERR_FAIL_NULL_V_MSG(voxel_room, nullptr, "Chunk node must be a direct child of VoxelRoom");

	Ref<VoxelRoomData> voxel_room_data = voxel_room->get_voxel_room_data();
	ERR_FAIL_NULL_V(voxel_room_data, nullptr);

	Ref<PresetsData> presets_data = voxel_room->get_presets_data();
	ERR_FAIL_NULL_V(presets_data, nullptr);

	Array basics_presets = presets_data->get_basics_presets();
	Array mesh_presets = presets_data->get_mesh_presets();
	Array device_presets = presets_data->get_device_presets();

	bool is_generate_basics = (filter & BASICS_FILTER) != 0;
	bool is_generate_mesh = (filter & MESH_FILTER) != 0;
	bool is_generate_device = (filter & DEVICE_FILTER) != 0;

	bool is_generate_transparent_true = (filter & TRANSPARENT_TRUE_FILTER) != 0;
	bool is_generate_transparent_false = (filter & TRANSPARENT_FALSE_FILTER) != 0;
	bool is_generate_transparent_all = is_generate_transparent_true == is_generate_transparent_false;

	bool is_generate_collider_true = (filter & COLLIDER_TRUE_FILTER) != 0;
	bool is_generate_collider_false = (filter & COLLIDER_FALSE_FILTER) != 0;
	bool is_generate_collider_all = is_generate_collider_true == is_generate_collider_false;

	PackedVector3Array collider_faces;
	Vector3i chunk_size = voxel_room_data->get_chunk_size();
	for (int x = 0; x < chunk_size.x; x++)
	{
		for (int y = 0; y < chunk_size.y; y++)
		{
			for (int z = 0; z < chunk_size.z; z++)
			{
				Vector3i local_position = Vector3i(x, y, z);
				Voxel voxel = get_voxel(local_position);
				int type = VoxelRoom::get_voxel_type(voxel);
				int id = VoxelRoom::get_voxel_id(voxel);
				if (type == VoxelRoomData::EMPTY)
				{
					continue;
				}
				else {
					Ref<Preset> preset;
					switch (type)
					{
					case VoxelRoomData::BASICS:
					{
						if (is_generate_basics == false)
						{
							continue;
						}
						ERR_FAIL_INDEX_V(id, basics_presets.size(), nullptr);
						preset = basics_presets[id];
						break;
					}
					case VoxelRoomData::MESH:
					{
						if (is_generate_mesh == false)
						{
							continue;
						}
						ERR_FAIL_INDEX_V(id, mesh_presets.size(), nullptr);
						preset = mesh_presets[id];
						break;
					}
					case VoxelRoomData::DEVICE:
					{
						if (is_generate_device == false)
						{
							continue;
						}
						ERR_FAIL_INDEX_V(id, device_presets.size(), nullptr);
						preset = device_presets[id];
					}
					}
					ERR_FAIL_NULL_V_MSG(preset, nullptr, "The preset is null [type = " + String::num_int64(type) + ",id = " + String::num_int64(id) + "]");
					if (is_generate_transparent_all == false)
					{
						if (is_generate_transparent_true == true && preset->get_transparent() == true)
						{
							continue;
						}
						if (is_generate_transparent_false == true && preset->get_transparent() == false)
						{
							continue;
						}
					}
					if (is_generate_collider_all == false)
					{
						if (is_generate_collider_true == true && preset->get_collider() == true)
						{
							continue;
						}
						if (is_generate_collider_false == true && preset->get_collider() == false)
						{
							continue;
						}
					}
					Vector3 vertexs[] = {
						Vector3(0.5, -0.5, -0.5) + Vector3(local_position),
						Vector3(0.5, 0.5, -0.5) + Vector3(local_position),
						Vector3(-0.5, 0.5, -0.5) + Vector3(local_position),
						Vector3(-0.5, -0.5, -0.5) + Vector3(local_position),
						Vector3(0.5, -0.5, 0.5) + Vector3(local_position),
						Vector3(0.5, 0.5, 0.5) + Vector3(local_position),
						Vector3(-0.5, 0.5, 0.5) + Vector3(local_position),
						Vector3(-0.5, -0.5, 0.5) + Vector3(local_position),
					};
					int up_voxel_type = VoxelRoom::get_voxel_type(get_voxel(local_position + Vector3i(0, 1, 0)));
					int down_voxel_type = VoxelRoom::get_voxel_type(get_voxel(local_position + Vector3i(0, -1, 0)));
					int front_voxel_type = VoxelRoom::get_voxel_type(get_voxel(local_position + Vector3i(0, 0, -1)));
					int back_voxel_type = VoxelRoom::get_voxel_type(get_voxel(local_position + Vector3i(0, 0, 1)));
					int left_voxel_type = VoxelRoom::get_voxel_type(get_voxel(local_position + Vector3i(-1, 0, 0)));
					int right_voxel_type = VoxelRoom::get_voxel_type(get_voxel(local_position + Vector3i(1, 0, 0)));

					if (up_voxel_type == VoxelRoomData::EMPTY)
					{
						collider_faces.push_back(vertexs[2]);
						collider_faces.push_back(vertexs[1]);
						collider_faces.push_back(vertexs[5]);
						collider_faces.push_back(vertexs[2]);
						collider_faces.push_back(vertexs[5]);
						collider_faces.push_back(vertexs[6]);
					}
					if (down_voxel_type == VoxelRoomData::EMPTY)
					{
						collider_faces.push_back(vertexs[0]);
						collider_faces.push_back(vertexs[3]);
						collider_faces.push_back(vertexs[7]);
						collider_faces.push_back(vertexs[0]);
						collider_faces.push_back(vertexs[7]);
						collider_faces.push_back(vertexs[4]);
					}
					if (front_voxel_type == VoxelRoomData::EMPTY)
					{
						collider_faces.push_back(vertexs[1]);
						collider_faces.push_back(vertexs[2]);
						collider_faces.push_back(vertexs[3]);
						collider_faces.push_back(vertexs[1]);
						collider_faces.push_back(vertexs[3]);
						collider_faces.push_back(vertexs[0]);
					}
					if (back_voxel_type == VoxelRoomData::EMPTY)
					{
						collider_faces.push_back(vertexs[6]);
						collider_faces.push_back(vertexs[5]);
						collider_faces.push_back(vertexs[4]);
						collider_faces.push_back(vertexs[6]);
						collider_faces.push_back(vertexs[4]);
						collider_faces.push_back(vertexs[7]);
					}
					if (left_voxel_type == VoxelRoomData::EMPTY)
					{
						collider_faces.push_back(vertexs[2]);
						collider_faces.push_back(vertexs[6]);
						collider_faces.push_back(vertexs[7]);
						collider_faces.push_back(vertexs[2]);
						collider_faces.push_back(vertexs[7]);
						collider_faces.push_back(vertexs[3]);
					}
					if (right_voxel_type == VoxelRoomData::EMPTY)
					{
						collider_faces.push_back(vertexs[5]);
						collider_faces.push_back(vertexs[1]);
						collider_faces.push_back(vertexs[0]);
						collider_faces.push_back(vertexs[5]);
						collider_faces.push_back(vertexs[0]);
						collider_faces.push_back(vertexs[4]);
					}

				}
			}
		}
	}

	Ref<ConcavePolygonShape3D> result;
	result.instantiate();
	if (collider_faces.size() > 0) {
		result->set_faces(collider_faces);
	}
	return result;
}

void Chunk::generate_device()
{
	VoxelRoom* voxel_room = get_voxel_room();
	Ref<VoxelRoomData> voxel_room_data = voxel_room->get_voxel_room_data();
	ERR_FAIL_NULL(voxel_room_data);

	Ref<PresetsData> presets_data = voxel_room->get_presets_data();
	Array device_presets = presets_data->get_device_presets();

	Vector3i chunk_size = voxel_room_data->get_chunk_size();
	for (int x = 0; x < chunk_size.x; x++)
	{
		for (int y = 0; y < chunk_size.y; y++)
		{
			for (int z = 0; z < chunk_size.z; z++)
			{
				Vector3i local_position = Vector3i(x, y, z);
				Voxel voxel = get_voxel(local_position);
				int type = VoxelRoom::get_voxel_type(voxel);
				int id = VoxelRoom::get_voxel_id(voxel);
				Vector3i rotation = VoxelRoom::get_voxel_rotation(voxel);

				Device* device = cast_to<Device>(devices[local_position]);

				if (type == VoxelRoomData::DEVICE)
				{
					Ref<DevicePreset> device_preset = device_presets[id];
					if (device != nullptr) {
						if (device->get_device_preset() != device_preset) {
							devices.erase(local_position);
							device->queue_free();
							device = nullptr;
						}
					}
					if (device == nullptr) {
						Node* node = device_preset->get_packed_scene()->instantiate();
						device = cast_to<Device>(node);
						if (unlikely(device == nullptr))
						{
							node->queue_free();
							ERR_FAIL_MSG("The DevicePreset with id " + String::num_int64(id) + " cannot be instantiated");
						}
						device->set_device_preset(device_preset);
						devices[local_position] = device;

						device->set_position(local_position);
						device->set_rotation(rotation);
						add_child(device);
					}
				}
				else {
					if (device != nullptr) {
						devices.erase(local_position);
						device->queue_free();
					}
				}
			}
		}
	}
}