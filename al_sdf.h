#ifndef AL_SDF_INCLUDED
#define AL_SDF_INCLUDED

/*
	al_sdf.h

	INCLUDE BEFORE THIS HEADER:
	- al_alloc.h
	- vecmath.h

	TODO:
	- multiple sdf scenes.
	  each sdf scene will have its own bvh tree?
	- BVH tree rotations ? most costly node reinsertion?
	- CLEANING!

	(12.08.2026)
	note that it is upto the user ot track all allocated shapes.
	alsdf does not provide a buffer which will list all of the existing sdfs.
	one could traverse the while bvh tree to do that, or simply keep the shape ids in a custom buffer.

	CODE SECTIONS:

	>>declaration
	- public declaration of functions & structs

	>>implementation
	- private implementation begining

	>>state
	- alsdf state

	>>update_aabb
	- incremental update system. tracks sdf shape updates and pushes them a list

	>>aabb_operations
	- operations on AABBs, like AABB union, intersection checks etc...

	>>aabb_calc
	- analytic and numerical calculation of a AABB of a SDF primitive

	>>aabb_bvh
	- incremental AABB Bounding Volume Hiearchy (BVH) implementation
	
	>>bvh_query
	- querying BVH nodes that intersect a point or a bounding volume

	>>aabb_updates
	- update functions that affect the BVH and incremental update list

	>>shape
	- alsdf shape interface implementation. functions for changing adding/removing SDF shape.
	
	>>dist_eq
	- distance equations for diffrent SDF primitives

	>>dist
	- functions for querying the distance of a SDF primitive or a SDF scene.

*/

//
// DECLARATION
// >>declaration

enum {
	AL_SDF_MAX_UPDATES = 16,
	AL_SDF_PARAMETER_COUNT = 8
};

// init/shutdown
void alsdf_init();
void alsdf_shutdown();

typedef struct alsdf_shape_id { uint32_t id; } alsdf_shape_id;
typedef struct alsdf_bvh_node_id { uint32_t id; } alsdf_bvh_node_id;

typedef struct alsdf_aabb {
	vec3_t lower_bound;
	vec3_t upper_bound;
} alsdf_aabb;

int alsdf_aabb_contains(alsdf_aabb aabb, vec3_t p);
int alsdf_aabb_intersects_aabb(alsdf_aabb aabb0, alsdf_aabb aabb1);
float alsdf_aabb_area(alsdf_aabb aabb);
alsdf_aabb alsdf_aabb_union(alsdf_aabb aabb0, alsdf_aabb aabb1);

typedef struct _alsdf_bvh_node {
	alsdf_aabb aabb;
	alsdf_bvh_node_id parent;
	alsdf_bvh_node_id children[2];
	alsdf_shape_id shape_id;
} _alsdf_bvh_node;

alsdf_bvh_node_id alsdf_get_bvh_root();
_alsdf_bvh_node* alsdf_get_bvh_node(alsdf_bvh_node_id node_id);
void alsdf_get_intersecting_bvh_nodes(vec3_t p, alsdf_bvh_node_id* intersecting_arr, int max_count, int* count);
void alsdf_get_intersecting_aabb_bvh_nodes(alsdf_aabb aabb, alsdf_bvh_node_id* intersecting_arr, int max_count, int* count);

typedef struct alsdf_updated_aabbs_range {
	alsdf_aabb* data;
	uint16_t count;
} alsdf_updated_aabbs_range;

alsdf_updated_aabbs_range alsdf_query_updated_aabbs();
void alsdf_clear_updated_aabbs();

typedef enum alsdf_shape_type {
	AL_SDF_TYPE_NONE = 0,
	AL_SDF_TYPE_CIRCLE,
	AL_SDF_TYPE_BOX,
	AL_SDF_TYPE_PENTAGRAM,
	AL_SDF_TYPE_LINE,
	_AL_SDF_TYPE_COUNT
} alsdf_shape_type;

typedef enum alsdf_shape_bool_operation_type {
	AL_SDF_OP_UNION = 0,
	AL_SDF_OP_SMOOTH_UNION,
	AL_SDF_OP_SUBTRACTION,
	AL_SDF_OP_SMOOTH_SUBTRACTION,
	AL_SDF_OP_INTERSECTION,
	AL_SDF_OP_XOR,
	_AL_SDF_OP_COUNT,
} alsdf_shape_bool_operation_type;

typedef enum alsdf_shape_parameter {
	AL_SDF_PARAM_CIRCLE_RADIUS = 0,
	AL_SDF_PARAM_BOX_WIDTH = 0,
	AL_SDF_PARAM_BOX_HEIGHT = 1,
	AL_SDF_PARAM_PENTAGRAM_SIZE = 0
} alsdf_shape_parameter;

typedef struct alsdf_shape_desc {
	vec3_t position;
	alsdf_shape_type type;
	vec3_t rotation;
	alsdf_shape_bool_operation_type operation_type;
	float parameters[AL_SDF_PARAMETER_COUNT];
} alsdf_shape_desc;

// add remove
alsdf_shape_id alsdf_add_shape(alsdf_shape_desc* desc);
void alsdf_remove_shape(alsdf_shape_id shape_id);

// set get
void alsdf_set_shape_position(alsdf_shape_id shape_id, vec3_t position);
vec3_t alsdf_get_shape_position(alsdf_shape_id shape_id);
void alsdf_set_shape_type(alsdf_shape_id shape_id, alsdf_shape_type type);
alsdf_shape_type alsdf_get_shape_type(alsdf_shape_id shape_id);
void alsdf_set_shape_operation_type(alsdf_shape_id shape_id, alsdf_shape_bool_operation_type type);
alsdf_shape_bool_operation_type alsdf_get_shape_operation_type(alsdf_shape_id shape_id);
void alsdf_set_shape_orientation(alsdf_shape_id shape_id, vec4_t orientation);
vec4_t alsdf_get_shape_orientation(alsdf_shape_id shape_id);
void alsdf_set_shape_smoothing(alsdf_shape_id shape_id, float smoothing);
float alsdf_get_shape_smoothing(alsdf_shape_id shape_id);
void alsdf_set_shape_parameter(alsdf_shape_id shape_id, alsdf_shape_parameter parameter, float value);
float alsdf_get_shape_parameter(alsdf_shape_id shape_id, alsdf_shape_parameter parameter);
alsdf_bvh_node_id alsdf_get_shape_bvh_node_id(alsdf_shape_id shape_id);
uint64_t alsdf_get_shape_alloc_id(alsdf_shape_id shape_id);

// specific set get functions for convenience
void alsdf_rotate_shape(alsdf_shape_id shape_id, vec3_t angles);
void alsdf_set_shape_rotation(alsdf_shape_id shape_id, vec3_t rotation);
vec3_t alsdf_get_shape_rotation(alsdf_shape_id shape_id);
alsdf_aabb alsdf_get_shape_aabb(alsdf_shape_id shape_id);
void alsdf_swap_shape_alloc_id(alsdf_shape_id shape0_id, alsdf_shape_id shape1_id);

// distance 
float alsdf_dist_shape(alsdf_shape_id shape_id, vec3_t p); // get distance to sdf shape 
float alsdf_shape_edit(alsdf_shape_id shape_id, float d, vec3_t p); // perform an sdf edit in a point in space with distance

// scene distance queries 
// (27.08.2026) odstranit?
float alsdf_dist(vec3_t p); // get distance quickly with BVH traversal. returns the true distance only when inside shapes aabb.

// intersection
alsdf_shape_id alsdf_intersecting_shape(vec3_t p); // také odstranit?
// ... ray intersection ?



#endif AL_SDF_INCLUDED
#define AL_IMPL
#ifdef AL_IMPL

//
// IMPLEMENTATION
// >>implementation

#define AL_ARRAY_SIZE(_arr) ((int)(sizeof(_arr) / sizeof(*(_arr))))

typedef struct _alsdf_shape {
	vec3_t position;
	alsdf_shape_type type;
	vec4_t orientation;
	alsdf_shape_bool_operation_type operation_type;
	float smoothing;
	float parameters[AL_SDF_PARAMETER_COUNT];
	uint16_t updated_aabb_id;						// index of the most relevant aabb region after update, if exists
	alsdf_bvh_node_id bvh_node_id;					// index of node in bvh node pool
	uint64_t alloc_id;								// allocation number, is uniqe for every shape and sets the order of sdfs.
} _alsdf_shape;

typedef struct _alsdf_updated_aabbs {
	alsdf_aabb data[AL_SDF_MAX_UPDATES];
	uint16_t count;
} _alsdf_updated_aabbs;

typedef struct _alsdf_affected_shapes {
	alsdf_shape_id data[AL_SDF_MAX_UPDATES];
	uint16_t count;
	// note that the count of _alsdf_affected_shapes is always smaller or equal to the count of _alsdf_updated_aabbs
	// since an affected shape is pushed into this stack when a fresh update is applied.
	// the stack is cleared based on the count of _alsdf_updated_aabbs.
} _alsdf_affected_shapes;

typedef struct _alsdf_updates {
	_alsdf_updated_aabbs updated_aabbs;		// list of aabbs which cover regions of space where the topology has changed
	_alsdf_affected_shapes affected_shapes;	// list of shapes that got changed, it is used for clearing their update_ids
} _alsdf_updates;

typedef struct _alsdf_bvh {
	al_bit_pool* nodes;
	alsdf_bvh_node_id root_node; // root is zero by default
} _alsdf_bvh;

typedef struct _alsdf_state {
	al_bit_pool* pool;
	uint64_t allocated_count;
	_alsdf_bvh bvh;
	_alsdf_updates updates;
	float max_smoothing;
} _alsdf_state;

static _alsdf_state _alsdf;

float _alsdf_dist_sdf(vec3_t p, alsdf_shape_type type, float* parameters); // (28.08.2026) temp, MESSY LIBRARY

//
// ALSDF STATE
// >>state

void alsdf_init() {
	_alsdf.pool = al_bit_pool_create(sizeof(_alsdf_shape), 2);
	_alsdf.bvh.nodes = al_bit_pool_create(sizeof(_alsdf_bvh_node), 2);
	_alsdf.max_smoothing = 0.1f;
}

void alsdf_shutdown() {
	al_bit_pool_delete(_alsdf.pool);
	_alsdf.pool = NULL;
	al_bit_pool_delete(_alsdf.bvh.nodes);
	_alsdf.bvh.nodes = NULL;
	_alsdf.bvh.root_node = (alsdf_bvh_node_id){ .id = 0 };
	_alsdf.updates.updated_aabbs.count = 0;
	_alsdf.updates.affected_shapes.count = 0;
}

_alsdf_shape* _alsdf_get_shape(alsdf_shape_id shape_id) {
	return (_alsdf_shape*)al_bit_pool_get(_alsdf.pool, (uint64_t)shape_id.id);
}

//
// UPDATED AABB
// >>update_aabb

alsdf_updated_aabbs_range alsdf_query_updated_aabbs() {
	alsdf_updated_aabbs_range range = {
		.data = _alsdf.updates.updated_aabbs.data,
		.count = _alsdf.updates.updated_aabbs.count
	};
	return range;
}

void alsdf_clear_updated_aabbs() {
	for (uint16_t i = 0; i < _alsdf.updates.affected_shapes.count; i++) { // reset states of affected stapes
		_alsdf_shape* shape = _alsdf_get_shape(_alsdf.updates.affected_shapes.data[i]);
		shape->updated_aabb_id = AL_SDF_MAX_UPDATES;
	}
	_alsdf.updates.updated_aabbs.count = 0;
	_alsdf.updates.affected_shapes.count = 0;
}

uint16_t _alsdf_add_updated_aabb() {
	if (_alsdf.updates.updated_aabbs.count >= AL_SDF_MAX_UPDATES) {
		printf("ERROR: alsdf.update_list reached max amount of updates! all updates will be cleared!\n");
		alsdf_clear_updated_aabbs(); // clear update stack when full, so we dont loose any handles to potentially allocated copies.
	}
	uint16_t update_index = _alsdf.updates.updated_aabbs.count++;
	_alsdf.updates.updated_aabbs.data[update_index] = (alsdf_aabb){ 0 };
	return update_index;
}

alsdf_aabb* _alsdf_get_updated_aabb(uint16_t update_index) {
	return &_alsdf.updates.updated_aabbs.data[update_index];
}
void _alsdf_push_affected_shape(alsdf_shape_id shape_id) {
	_alsdf.updates.affected_shapes.data[_alsdf.updates.affected_shapes.count++] = shape_id;
}

void _alsdf_push_update_add(alsdf_shape_id shape_id) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	// first time shape was created so always push updated aabb
	shape->updated_aabb_id = _alsdf_add_updated_aabb();
	_alsdf_push_affected_shape(shape_id);
	*_alsdf_get_updated_aabb(shape->updated_aabb_id) = alsdf_get_shape_aabb(shape_id);
}

void _alsdf_push_update_remove(alsdf_shape_id shape_id) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	// if shape was not affected yet, add only outdated aabb, since it will no longer exist if updated
	if (shape->updated_aabb_id == AL_SDF_MAX_UPDATES) {
		uint16_t outdated_region_id = _alsdf_add_updated_aabb();
		*_alsdf_get_updated_aabb(outdated_region_id) = alsdf_get_shape_aabb(shape_id);
	}
}

void _alsdf_push_update_value(alsdf_shape_id shape_id, alsdf_aabb outdated_aabb, alsdf_aabb updated_aabb) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	// if updated first time
	if (shape->updated_aabb_id == AL_SDF_MAX_UPDATES) {
		// push outdated aabb to update stack, and set value
		uint16_t outdated_region_id = _alsdf_add_updated_aabb();
		*_alsdf_get_updated_aabb(outdated_region_id) = outdated_aabb;
		// add updated aabb to update stack
		shape->updated_aabb_id = _alsdf_add_updated_aabb();
		_alsdf_push_affected_shape(shape_id);
	}
	// set updated aabb value
	*_alsdf_get_updated_aabb(shape->updated_aabb_id) = updated_aabb;
}
//
// AABB OPERATIONS
// >>aabb_operations
//
// resources:
// - https://gpfault.net/posts/aabb-tricks.html (nice blog, although we use a slightly diffrent aabb structure here)

int alsdf_aabb_contains(alsdf_aabb aabb, vec3_t p) {
	return (
		aabb.lower_bound.x <= p.x &&
		aabb.lower_bound.y <= p.y &&
		aabb.lower_bound.z <= p.z &&
		aabb.upper_bound.x >= p.x &&
		aabb.upper_bound.y >= p.y &&
		aabb.upper_bound.z >= p.z
	);
}

int alsdf_aabb_intersects_aabb(alsdf_aabb aabb0, alsdf_aabb aabb1) {
	return (
		aabb0.lower_bound.x <= aabb1.upper_bound.x &&
		aabb0.lower_bound.y <= aabb1.upper_bound.y &&
		aabb0.lower_bound.z <= aabb1.upper_bound.z &&
		aabb0.upper_bound.x >= aabb1.lower_bound.x &&
		aabb0.upper_bound.y >= aabb1.lower_bound.y &&
		aabb0.upper_bound.z >= aabb1.lower_bound.z
	);
}

float alsdf_aabb_area(alsdf_aabb aabb) {
	float x = aabb.upper_bound.x - aabb.lower_bound.x;
	float y = aabb.upper_bound.y - aabb.lower_bound.y;
	float z = aabb.upper_bound.z - aabb.lower_bound.z;
	return  (x * y + x * z + y * z) * 2.0f;
}

alsdf_aabb alsdf_aabb_union(alsdf_aabb aabb0, alsdf_aabb aabb1) {
	return (alsdf_aabb) {
		.lower_bound = vec3_min(aabb0.lower_bound, aabb1.lower_bound),
		.upper_bound = vec3_max(aabb0.upper_bound, aabb1.upper_bound),
	};
}

//
// AABB CALCULATION
// >>aabb_calc

#define AL_SDF_MAX_BOUND_DISTANCE (64.0f)

alsdf_aabb _alsdf_aabb_circle(float radius) {
	return (alsdf_aabb) {
		.lower_bound = vec3(-radius, -radius, 0.0f), // (07.08.2026) z component je pro 2d buï + nekoneèno, nebo - nekoneèno. ve 3d to dává smysl.
		.upper_bound = vec3(radius, radius, 0.0f),
	};
}

alsdf_aabb _alsdf_aabb_numerical(alsdf_shape_id shape_id) {
	const float sample_distance = AL_SDF_MAX_BOUND_DISTANCE; // how far is sdf sampeled, also determines the aabb of 2D sampeled shape in the z axis
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	// sample points
	vec3_t pos_x = quat_rotate_vector(vec3(sample_distance, 0.0f, 0.0f), shape->orientation);
	vec3_t pos_y = quat_rotate_vector(vec3(0.0f, sample_distance, 0.0f), shape->orientation);
	vec3_t pos_z = quat_rotate_vector(vec3(0.0f, 0.0f, sample_distance), shape->orientation);
	vec3_t neg_x = quat_rotate_vector(vec3(-sample_distance, 0.0f, 0.0f), shape->orientation);
	vec3_t neg_y = quat_rotate_vector(vec3(0.0f, -sample_distance, 0.0f), shape->orientation);
	vec3_t neg_z = quat_rotate_vector(vec3(0.0f, 0.0f, -sample_distance), shape->orientation);
	// calculate aabb based on sampeled distances
	return (alsdf_aabb) {
		.lower_bound = {
			.x = _alsdf_dist_sdf(neg_x, shape->type, shape->parameters) - sample_distance,
			.y = _alsdf_dist_sdf(neg_y, shape->type, shape->parameters) - sample_distance,
			.z = 0.0f//_alsdf_dist_sdf(neg_z, shape->type, shape->parameters) - sample_distance
		},
			.upper_bound = {
				.x = sample_distance - _alsdf_dist_sdf(pos_x, shape->type, shape->parameters),
				.y = sample_distance - _alsdf_dist_sdf(pos_y, shape->type, shape->parameters),
				.z = 0.0f//sample_distance - _alsdf_dist_sdf(pos_z, shape->type, shape->parameters)
		},
	};
}

alsdf_aabb _alsdf_calc_shape_aabb(alsdf_shape_id shape_id) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	alsdf_aabb aabb = (alsdf_aabb){ 0 };
	// get shape bound
	switch (shape->type) {
	case AL_SDF_TYPE_CIRCLE:
		aabb = _alsdf_aabb_circle(shape->parameters[AL_SDF_PARAM_CIRCLE_RADIUS]);
		break;
	default: // if no analytic solution exists, calculate aabb numerically
		aabb = _alsdf_aabb_numerical(shape_id);
		break;
	}
	// offset bounds to shape position
	aabb.lower_bound = vec3_add(aabb.lower_bound, shape->position);
	aabb.upper_bound = vec3_add(aabb.upper_bound, shape->position);

	// expand every aabb by smoothing factor. (NOTE: not optimal)
	aabb.lower_bound = vec3_subf(aabb.lower_bound, _alsdf.max_smoothing);
	aabb.upper_bound = vec3_addf(aabb.upper_bound, _alsdf.max_smoothing);

	// (18.08.2026) temp fix for clamped values on edges of aabb
	float padding = 2.0f / 64.0f;
	aabb.lower_bound = vec3_subf(aabb.lower_bound, padding);
	aabb.upper_bound = vec3_addf(aabb.upper_bound, padding);

	return aabb;
}

//
// AABB BVH
// >>aabb_bvh
//
// references:
// - https://box2d.org/files/ErinCatto_DynamicBVH_Full.pdf
// - https://dcgi.fel.cvut.cz/en/publications/2013/bittner-cgf-fiobvh/
// - https://dcgi.fel.cvut.cz/home/bittner/publications/cag2014.pdf

alsdf_bvh_node_id alsdf_get_bvh_root() {
	return _alsdf.bvh.root_node;
}

_alsdf_bvh_node* alsdf_get_bvh_node(alsdf_bvh_node_id node_id) {
	return (_alsdf_bvh_node*)al_bit_pool_get(_alsdf.bvh.nodes, node_id.id);
}

alsdf_bvh_node_id _alsdf_bvh_find_best_node_position(alsdf_aabb aabb) {
	alsdf_bvh_node_id node_id = _alsdf.bvh.root_node;
	_alsdf_bvh_node* node = alsdf_get_bvh_node(node_id);
	// all leaf nodes have a primitive, so we will allways exit from the loop
	while (true) {
		// exit if at the end of a tree
		if (node->shape_id.id != 0) {
			break;
		}
		// note if node doesnt hold a primitive, it will always have 2 valid children!
		_alsdf_bvh_node* child_node0 = alsdf_get_bvh_node(node->children[0]);
		_alsdf_bvh_node* child_node1 = alsdf_get_bvh_node(node->children[1]);
		// TODO: not sure on the branch cost...? whats the cost of splitting? when is the cost 0 to split? THIS IS JUST A GUESS!!
		// if the cost model was 100% correct, the total cost of the tree would change linearly without any jumps when moving an object around.
		float branch_cost = alsdf_aabb_area(node->aabb);
		// cost of children is the increase of the childs aabb if a union were applied whit the inserted node.
		float branch_child0_cost = alsdf_aabb_area(alsdf_aabb_union(child_node0->aabb, aabb)) - alsdf_aabb_area(child_node0->aabb); // is zero if best, if the inserted node is inside the node
		float branch_child1_cost = alsdf_aabb_area(alsdf_aabb_union(child_node1->aabb, aabb)) - alsdf_aabb_area(child_node1->aabb);
		// exit search if it is less costly to branch now.
		// note: if branch_cost == child_branch_cost, then always go to children!
		if ((branch_cost < branch_child0_cost) && (branch_cost < branch_child1_cost)) {
			break;
		}
		// refit aabb
		node->aabb = alsdf_aabb_union(node->aabb, aabb);
		// find least costly child node and iterate in it.
		uint8_t best_child = 0;
		if (branch_child1_cost < branch_child0_cost) best_child = 1;
		node_id = node->children[best_child];
		node = alsdf_get_bvh_node(node_id);
	}
	return node_id;
}

void _alsdf_bvh_make_siblings(alsdf_bvh_node_id node_id, alsdf_bvh_node_id inserted_node_id) {
	_alsdf_bvh_node* node = alsdf_get_bvh_node(node_id);
	alsdf_bvh_node_id new_parent_node_id = (alsdf_bvh_node_id){
		.id = (uint32_t)al_bit_pool_add(_alsdf.bvh.nodes)
	}; // create new parent
	_alsdf_bvh_node* new_parent_node = alsdf_get_bvh_node(new_parent_node_id);
	new_parent_node->parent = node->parent;
	if (node->parent.id != 0) { // update previous parent
		_alsdf_bvh_node* prev_parent_node = alsdf_get_bvh_node(node->parent);
		if (prev_parent_node->children[0].id == node_id.id) prev_parent_node->children[0] = new_parent_node_id;
		else prev_parent_node->children[1] = new_parent_node_id;
	}
	else { // if expanded node was root, set its parent as new root.
		_alsdf.bvh.root_node = new_parent_node_id;
	}
	new_parent_node->children[0] = node_id; // set new parent for node
	node->parent = new_parent_node_id;
	_alsdf_bvh_node* inserted_node = alsdf_get_bvh_node(inserted_node_id); // set new parent for inserted node
	new_parent_node->children[1] = inserted_node_id;
	inserted_node->parent = new_parent_node_id;
	new_parent_node->aabb = alsdf_aabb_union(node->aabb, inserted_node->aabb); // refit aabb
}

void _alsdf_bvh_insert_node(alsdf_bvh_node_id inserted_node_id) {
	if (_alsdf.bvh.root_node.id == 0) {
		_alsdf.bvh.root_node = inserted_node_id;
		return;
	}
	_alsdf_bvh_node* inserted_node = alsdf_get_bvh_node(inserted_node_id);
	alsdf_bvh_node_id best_node_id = _alsdf_bvh_find_best_node_position(inserted_node->aabb);
	_alsdf_bvh_make_siblings(best_node_id, inserted_node_id);
}

alsdf_bvh_node_id _alsdf_bvh_cut_sibling(alsdf_bvh_node_id node_id) {
	_alsdf_bvh_node* node = alsdf_get_bvh_node(node_id);
	if (node->parent.id == 0) { // node is root node, remove root.
		_alsdf.bvh.root_node.id = 0;
		return (alsdf_bvh_node_id) { .id = 0 };
	}
	_alsdf_bvh_node* parent = alsdf_get_bvh_node(node->parent);
	uint8_t sibling_index = 0;
	if (parent->children[0].id == node_id.id) sibling_index = 1;
	alsdf_bvh_node_id sibling_id = parent->children[sibling_index]; // get sibling_id
	_alsdf_bvh_node* sibling = alsdf_get_bvh_node(sibling_id);
	if (parent->parent.id == 0) { // if parent of parent is root, set root to parent
		_alsdf.bvh.root_node = sibling_id;
	}
	else { // else update children of parent of parent 
		_alsdf_bvh_node* parent_of_parent = alsdf_get_bvh_node(parent->parent);
		uint8_t child_index = 0;
		if (parent_of_parent->children[1].id == node->parent.id) child_index = 1;
		parent_of_parent->children[child_index] = sibling_id;
	}
	sibling->parent = parent->parent;
	al_bit_pool_remove(_alsdf.bvh.nodes, node->parent.id); // deallocate uneccessary node
	return sibling_id; // return sibling, who took the position of parent
}

void _alsdf_bvh_refit_ancestors(alsdf_bvh_node_id node_id) {
	while (node_id.id != 0) { // update up to the root (parent of root is 0)
		_alsdf_bvh_node* node = alsdf_get_bvh_node(node_id);
		alsdf_aabb child0_aabb = alsdf_get_bvh_node(node->children[0])->aabb;
		alsdf_aabb child1_aabb = alsdf_get_bvh_node(node->children[1])->aabb;
		node->aabb = alsdf_aabb_union(child0_aabb, child1_aabb);
		node_id = node->parent;
	}
}

void _alsdf_bvh_remove_node(alsdf_bvh_node_id node_id) { // remove node from bvh withoud deleting it. node_id will be still valid.
	_alsdf_bvh_node* node = alsdf_get_bvh_node(node_id);
	alsdf_bvh_node_id sibling_id = _alsdf_bvh_cut_sibling(node_id);
	if (sibling_id.id == 0) return; // if had no siblings, i.e. node was the root. the node was still removed.
	_alsdf_bvh_node* sibling = alsdf_get_bvh_node(node_id);
	_alsdf_bvh_refit_ancestors(sibling->parent);
}

void _alsdf_bvh_update_node(alsdf_bvh_node_id node_id) {
	_alsdf_bvh_remove_node(node_id);
	_alsdf_bvh_insert_node(node_id);
}

//
// BVH QUERY
// >>bvh_query

void _alsdf_sort_bvh_nodes(alsdf_bvh_node_id* intersecting_arr, int max_count, int* count) {
	// bubble sort stack based on alloc_id (alloc_id must be in ascending order)
	if (*count == 0) return;
	bool changed = true;
	while (changed) {
		changed = false;
		for (int i = 0; i < *count - 1; i++) {
			// (10.08.2026) kinda messy
			_alsdf_bvh_node* node0 = alsdf_get_bvh_node(intersecting_arr[i]);
			_alsdf_shape* shape0 = _alsdf_get_shape(node0->shape_id);
			_alsdf_bvh_node* node1 = alsdf_get_bvh_node(intersecting_arr[i + 1]);
			_alsdf_shape* shape1 = _alsdf_get_shape(node1->shape_id);
			if (shape0->alloc_id > shape1->alloc_id) {
				alsdf_bvh_node_id intermediate = intersecting_arr[i];
				intersecting_arr[i] = intersecting_arr[i + 1];
				intersecting_arr[i + 1] = intermediate;
				changed = true;
			}
		}
	}
}

void alsdf_get_intersecting_bvh_nodes(vec3_t p, alsdf_bvh_node_id* intersecting_arr, int max_count, int* count) {
	if (_alsdf.bvh.root_node.id == 0) return;
	alsdf_bvh_node_id stack[64] = { 0 };
	int stack_count = 0;
	stack[stack_count++] = _alsdf.bvh.root_node;
	// get all intersecting nodes
	while ((stack_count > 0) && ((*count) < max_count) && (stack_count < AL_ARRAY_SIZE(stack))) {
		// get node and pop stack
		alsdf_bvh_node_id node_id = stack[--stack_count];
		_alsdf_bvh_node* node = alsdf_get_bvh_node(node_id);
		// if reached bottom push to collision stack and continue to next nodes in traversal stack
		if ((node->shape_id.id != 0)) {
			intersecting_arr[(*count)++] = node_id;
			continue;
		}
		// push children to stack if valid
		for (int i = 0; i < 2; i++) {
			_alsdf_bvh_node* child = alsdf_get_bvh_node(node->children[i]);
			if (alsdf_aabb_contains(child->aabb, p)) {
				stack[stack_count++] = node->children[i];
			}
		}
	}
	_alsdf_sort_bvh_nodes(intersecting_arr, max_count, count); // always keep sdfs in order
}

void alsdf_get_intersecting_aabb_bvh_nodes(alsdf_aabb aabb, alsdf_bvh_node_id* intersecting_arr, int max_count, int* count) {
	if (_alsdf.bvh.root_node.id == 0) return;
	alsdf_bvh_node_id stack[64] = { 0 };
	int stack_count = 0;
	stack[stack_count++] = _alsdf.bvh.root_node;
	// get all intersecting nodes
	while ((stack_count > 0) && ((*count) < max_count) && (stack_count < AL_ARRAY_SIZE(stack))) {
		// get node and pop stack
		alsdf_bvh_node_id node_id = stack[--stack_count];
		_alsdf_bvh_node* node = alsdf_get_bvh_node(node_id);
		// if reached bottom push to collision stack and continue to next nodes in traversal stack
		if ((node->shape_id.id != 0)) {
			intersecting_arr[(*count)++] = node_id;
			continue;
		}
		// push children to stack if valid
		for (int i = 0; i < 2; i++) {
			_alsdf_bvh_node* child = alsdf_get_bvh_node(node->children[i]);
			if (alsdf_aabb_intersects_aabb(child->aabb, aabb)) {
				stack[stack_count++] = node->children[i];
			}
		}
	}
	_alsdf_sort_bvh_nodes(intersecting_arr, max_count, count); // always keep sdfs in order
}

//
// AABB UPDATES
// >>aabb_updates
// an aabb update affects the updated regions stack and the bvh.

void _alsdf_create_shape_aabb(alsdf_shape_id shape_id) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	alsdf_bvh_node_id bvh_node_id = (alsdf_bvh_node_id){
		.id = (uint32_t)al_bit_pool_add(_alsdf.bvh.nodes)
	}; // create bvh node
	_alsdf_bvh_node* bvh_node = alsdf_get_bvh_node(bvh_node_id);
	*bvh_node = (_alsdf_bvh_node){
		.shape_id = shape_id,
		.aabb = _alsdf_calc_shape_aabb(shape_id)
	};
	_alsdf_bvh_insert_node(bvh_node_id); // add to bvh
	shape->bvh_node_id = bvh_node_id; // note that bvh_node_id never changes until shape is deleted
	_alsdf_push_update_add(shape_id); // push update to update stack
}

void _alsdf_remove_shape_aabb(alsdf_shape_id shape_id) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	// remove from bvh
	_alsdf_bvh_remove_node(shape->bvh_node_id);
	al_bit_pool_remove(_alsdf.bvh.nodes, shape->bvh_node_id.id); // also deallocate the node

	_alsdf_push_update_remove(shape_id);
}

void _alsdf_update_shape_aabb(alsdf_shape_id shape_id) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	_alsdf_bvh_node* bvh_node = alsdf_get_bvh_node(shape->bvh_node_id);
	// push a region update
	alsdf_aabb outdated_aabb = bvh_node->aabb;
	bvh_node->aabb = _alsdf_calc_shape_aabb(shape_id);
	_alsdf_push_update_value(shape_id, outdated_aabb, bvh_node->aabb);
	// incrementally update the bvh
	_alsdf_bvh_update_node(shape->bvh_node_id);
}

//
// SHAPE OPERATIONS
// >>shape

alsdf_shape_id alsdf_add_shape(alsdf_shape_desc* desc) {
	alsdf_shape_id shape_id = {
		.id = (uint32_t)al_bit_pool_add(_alsdf.pool)
	};
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	*shape = (_alsdf_shape){
		.type = desc->type,
		.position = desc->position,
		.operation_type = desc->operation_type,
		.smoothing = 0.0f,
		.updated_aabb_id = AL_SDF_MAX_UPDATES,
		.bvh_node_id = 0, // will be set in _alsdf_create_shape_aabb()
		.alloc_id = _alsdf.allocated_count++
	};
	shape->orientation = quat_identity();
	shape->orientation = quat_mul(shape->orientation, quat_rotation_yaw_pitch_roll(desc->rotation.x, desc->rotation.y, desc->rotation.z));
	for (int i = 0; i < AL_SDF_PARAMETER_COUNT; i++) {
		shape->parameters[i] = desc->parameters[i] != 0.0f ? desc->parameters[i] : 0.2f;
	}
	_alsdf_create_shape_aabb(shape_id); // create aabb for shape in the bvh
	return shape_id;
}

void alsdf_remove_shape(alsdf_shape_id shape_id) {
	_alsdf_remove_shape_aabb(shape_id); // remove aabb from bvh
	al_bit_pool_remove(_alsdf.pool, shape_id.id); // deallocate shape. handle is now invalid
}

void alsdf_set_shape_position(alsdf_shape_id shape_id, vec3_t position) {
	_alsdf_get_shape(shape_id)->position = position;
	_alsdf_update_shape_aabb(shape_id);
}

vec3_t alsdf_get_shape_position(alsdf_shape_id shape_id) {
	return _alsdf_get_shape(shape_id)->position;
}

void alsdf_set_shape_type(alsdf_shape_id shape_id, alsdf_shape_type type) {
	_alsdf_get_shape(shape_id)->type = type;
	_alsdf_update_shape_aabb(shape_id);
}

alsdf_shape_type alsdf_get_shape_type(alsdf_shape_id shape_id) {
	return _alsdf_get_shape(shape_id)->type;
}

void alsdf_set_shape_operation_type(alsdf_shape_id shape_id, alsdf_shape_bool_operation_type type) {
	_alsdf_get_shape(shape_id)->operation_type = type;
	_alsdf_update_shape_aabb(shape_id);
}
alsdf_shape_bool_operation_type alsdf_get_shape_operation_type(alsdf_shape_id shape_id) {
	return _alsdf_get_shape(shape_id)->operation_type;
}

void alsdf_set_shape_orientation(alsdf_shape_id shape_id, vec4_t orientation) {
	_alsdf_get_shape(shape_id)->orientation = orientation;
	_alsdf_update_shape_aabb(shape_id);
}

vec4_t alsdf_get_shape_orientation(alsdf_shape_id shape_id) {
	return _alsdf_get_shape(shape_id)->orientation;
}

void alsdf_set_shape_smoothing(alsdf_shape_id shape_id, float smoothing) {
	_alsdf_get_shape(shape_id)->smoothing = smoothing;
	_alsdf_update_shape_aabb(shape_id);
}

float alsdf_get_shape_smoothing(alsdf_shape_id shape_id) {
	return _alsdf_get_shape(shape_id)->smoothing;
}

void alsdf_set_shape_parameter(alsdf_shape_id shape_id, alsdf_shape_parameter parameter, float value) {
	_alsdf_get_shape(shape_id)->parameters[parameter] = value;
	_alsdf_update_shape_aabb(shape_id);
}

float alsdf_get_shape_parameter(alsdf_shape_id shape_id, alsdf_shape_parameter parameter) {
	return _alsdf_get_shape(shape_id)->parameters[parameter];
}

alsdf_bvh_node_id alsdf_get_shape_bvh_node_id(alsdf_shape_id shape_id) {
	return _alsdf_get_shape(shape_id)->bvh_node_id;
}

uint64_t alsdf_get_shape_alloc_id(alsdf_shape_id shape_id) {
	return _alsdf_get_shape(shape_id)->alloc_id;
}

void alsdf_rotate_shape(alsdf_shape_id shape_id, vec3_t angles) {
	vec4_t* orientation = &_alsdf_get_shape(shape_id)->orientation;
	*orientation = quat_mul(*orientation, quat_rotation_yaw_pitch_roll(angles.x, angles.y, angles.z));
	_alsdf_update_shape_aabb(shape_id);
}

void alsdf_set_shape_rotation(alsdf_shape_id shape_id, vec3_t rotation) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	shape->orientation = quat_identity();
	shape->orientation = quat_mul(shape->orientation, quat_rotation_yaw_pitch_roll(rotation.x, rotation.y, rotation.z));
	_alsdf_update_shape_aabb(shape_id);
}

vec3_t alsdf_get_shape_rotation(alsdf_shape_id shape_id) {
	vec4_t q = alsdf_get_shape_orientation(shape_id);
	// thanks: https://discuss.luxonis.com/d/5453-how-to-convert-quaternions-to-pitchrollyaw/2
	float roll = vecmath_atan2(2 * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y));
	float pitch = vecmath_asin(2 * (q.w * q.y - q.z * q.x));
	float yaw = vecmath_atan2(2 * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y * q.y + q.z * q.z));
	return vec3(roll, pitch, yaw);
}

alsdf_aabb alsdf_get_shape_aabb(alsdf_shape_id shape_id) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	_alsdf_bvh_node* bvh_node = alsdf_get_bvh_node(shape->bvh_node_id);
	return bvh_node->aabb; // note that if shape_id or shape->bvh_node_id would be invalid, aabb will have null value, i.e. aabb that has its bounds at zero.
}

void alsdf_swap_shape_alloc_id(alsdf_shape_id shape0_id, alsdf_shape_id shape1_id) {
	uint64_t temp_alloc_id = _alsdf_get_shape(shape0_id)->alloc_id;
	_alsdf_get_shape(shape0_id)->alloc_id = _alsdf_get_shape(shape1_id)->alloc_id;
	_alsdf_get_shape(shape1_id)->alloc_id = temp_alloc_id;
	_alsdf_update_shape_aabb(shape0_id);
	_alsdf_update_shape_aabb(shape1_id);
}

//
// DISTANCE EQUATIONS
// >>dist_eq

float _alsdf_dist_circle(vec2_t p, float radius) {
	return vec2_length(p) - radius;
}

float _alsdf_dist_box(vec2_t p, vec2_t b) {
	vec2_t d = vec2_sub(vec2_abs(p), b);
	return vec2_length(vec2_max(d, vec2f(0.0))) + vecmath_min(vecmath_max(d.x, d.y), 0.0);
}

float _alsdf_dist_pentagram(vec2_t p, float r) {
	const float k1x = 0.809016994f;
	const float k2x = 0.309016994f;
	const float k1y = 0.587785252f;
	const float k2y = 0.951056516f;
	const float k1z = 0.726542528f;
	const vec2_t  v1 = vec2(k1x, -k1y);
	const vec2_t  v2 = vec2(-k1x, -k1y);
	const vec2_t  v3 = vec2(k2x, -k2y);
	p.x = vecmath_abs(p.x);
	p = vec2_sub(p, vec2_fmul(2.0f * vecmath_max(vec2_dot(v1, p), 0.0), v1));
	p = vec2_sub(p, vec2_fmul(2.0f * vecmath_max(vec2_dot(v2, p), 0.0), v2));
	p.x = vecmath_abs(p.x);
	p.y -= r;
	return vec2_length(vec2_sub(p, vec2_mulf(v3, vecmath_clamp(vec2_dot(p, v3), 0.0, k1z * r)))) * vecmath_sign(p.y * v3.x - p.x * v3.y);
}

float _alsdf_dist_line(vec2_t p, vec2_t a, vec2_t b, float r) {
	vec2_t ba = vec2_sub(b, a);
	vec2_t pa = vec2_sub(p, a);
	float h = vecmath_clamp(vec2_dot(pa, ba) / vec2_dot(ba, ba), 0.0f, 1.0f);
	return vec2_length(vec2_sub(pa, vec2_mulf(ba, h))) - r;
}


//
// DISTANCE QUERY
// >>dist

float _alsdf_dist_sdf(vec3_t p, alsdf_shape_type type, float* parameters) {
	float d = 1024.0f;
	switch (type) {
	case AL_SDF_TYPE_CIRCLE:
		d = _alsdf_dist_circle(vec3_xy(p), parameters[AL_SDF_PARAM_CIRCLE_RADIUS]);
		break;
	case AL_SDF_TYPE_BOX:
		d = _alsdf_dist_box(vec3_xy(p), vec2(parameters[AL_SDF_PARAM_BOX_WIDTH], parameters[AL_SDF_PARAM_BOX_HEIGHT]));
		break;
	case AL_SDF_TYPE_PENTAGRAM:
		d = _alsdf_dist_pentagram(vec3_xy(p), parameters[AL_SDF_PARAM_PENTAGRAM_SIZE]);
		break;
	case AL_SDF_TYPE_LINE:
		d = _alsdf_dist_line(vec3_xy(p), vec2(parameters[1], parameters[2]), vec2(parameters[3], parameters[4]), parameters[0]);
		break;
	}
	return d;
}

vec3_t _alsdf_transform_point(vec3_t p, vec3_t position, vec4_t orientation) {
	return quat_rotate_vector(vec3_sub(p, position), orientation);
}

float alsdf_dist_shape(alsdf_shape_id shape_id, vec3_t p) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	p = _alsdf_transform_point(p, shape->position, shape->orientation);
	return _alsdf_dist_sdf(p, shape->type, shape->parameters);
}

float _alsdf_op_smooth_union(float a, float b, float k) {
	k = vecmath_clamp(k, 0.0f, _alsdf.max_smoothing); // clamp to max smoothing
	k += 0.001f; // epsilon value guarantees we dont divide by 0
	float h = vecmath_max(k - vecmath_abs(a - b), 0.0f);
	return vecmath_min(a, b) - h * h * 0.25f / k;
}

float _alsdf_op_smooth_subtraction(float a, float b, float k) {
	return -_alsdf_op_smooth_subtraction(-a, b, k);
}

float alsdf_shape_edit(alsdf_shape_id shape_id, float d, vec3_t p) {
	_alsdf_shape* shape = _alsdf_get_shape(shape_id);
	float d_shape = alsdf_dist_shape(shape_id, p);
	// perform sdf operation
	switch (shape->operation_type) {
	case AL_SDF_OP_UNION:
		d = vecmath_min(d, d_shape);
		break;
	case AL_SDF_OP_SMOOTH_UNION:
		d = _alsdf_op_smooth_union(d, d_shape, shape->smoothing);
		break;
	case AL_SDF_OP_SUBTRACTION:
		d = vecmath_max(d, -d_shape);
		break;
	case AL_SDF_OP_SMOOTH_SUBTRACTION:
		d = _alsdf_op_smooth_subtraction(d, d_shape, shape->smoothing);
		break;
	case AL_SDF_OP_INTERSECTION:
		d = vecmath_max(d, d_shape);
		break;
	case AL_SDF_OP_XOR:
		d = vecmath_max(vecmath_min(d, d_shape), -vecmath_max(d, d_shape));
		break;
	}
	return d;
}


// (27.08.2026) nehodí se do alsdf ?
float alsdf_dist(vec3_t p) { // get closest shape distance in O(log2(n))
	// get sorted stack of colliding primitives, sorted based on allocation_id which determines the order of sdf edits
	alsdf_bvh_node_id collision_stack[64] = { 0 };
	int collision_stack_count = 0;
	alsdf_get_intersecting_bvh_nodes(p, collision_stack, AL_ARRAY_SIZE(collision_stack), &collision_stack_count);
	// perform sdf edits on intersecting sdfs
	float d = 1024.0f;
	for (int i = 0; i < collision_stack_count; i++) {
		_alsdf_bvh_node* node = alsdf_get_bvh_node(collision_stack[i]);
		d = alsdf_shape_edit(node->shape_id, d, p);
	}
	return d;
}

// (27.08.2026) nehodí se do alsdf ?
alsdf_shape_id alsdf_intersecting_shape(vec3_t p) {
	alsdf_bvh_node_id collision_stack[64] = { 0 };
	int collision_stack_count = 0;
	alsdf_get_intersecting_bvh_nodes(p, collision_stack, AL_ARRAY_SIZE(collision_stack), &collision_stack_count);
	// get intersecting shape based on its allocation id. i.e. will pick the latest shape in ordered list of sdfs.
	// this works nicely for object picking. alternatively you could get the shape based on its proximity to p, but that doesnt work great for object picking.
	float d = 1024.0f;
	uint64_t biggest_alloc_id = 0;
	alsdf_shape_id closest_shape = { .id = 0 };
	for (int i = 0; i < collision_stack_count; i++) {
		alsdf_shape_id intersecting_shape = alsdf_get_bvh_node(collision_stack[i])->shape_id;
		// set all shapes to be the same size, this is very usefull for object picking.
		float d_shape = alsdf_dist_shape(intersecting_shape, p);
		uint64_t alloc_id = alsdf_get_shape_alloc_id(intersecting_shape);
		if (alloc_id >= biggest_alloc_id && d_shape <= 0.0f) {
			d = d_shape;
			closest_shape = intersecting_shape;
			biggest_alloc_id = alloc_id;
		}
	}
	
	return closest_shape;
}

#endif AL_IMPL