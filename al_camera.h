#ifndef AL_CAMERA_INCLUDED
#define AL_CAMERA_INCLUDED

/*
* 
	al_camera.h

	camera can be used both in 2D & 3D with diffrent modes.
	camera also supports custom behaviour.

	references:
	- https://github.com/Crydsch/camera/blob/main/camera.h

	INCLUDE BEFORE:
	- al_input.h

	TODO:
	- interpolation?

*/

typedef struct alcam_view { // view related data
	float fov; // in degrees
	float near_plane;
	float far_plane;
	float target_distance; // useful for orbital camera
} alcam_view;

typedef struct alcam_movement { // movement related data
	float sensitivity;
	float move_speed;
	float sprint_speed;
} alcam_movement;

typedef void (*alcam_movement_function)(struct al_camera*, float);

typedef struct alcam_mode {
	struct {
		bool is_2d;
		bool has_lookat_target;
	} attributes;
	alcam_movement_function movement_function;
} alcam_mode;

typedef struct al_camera {
	vec3_t position; 
	vec4_t orientation; // quaternion of orientation in space
	vec3_t up;
	vec3_t scale; 
	mat44_t matrix;
	alcam_view view;
	alcam_movement movement;
	alcam_mode mode;
} al_camera;

typedef struct al_camera_desc_t {
	vec3_t position;
	vec3_t angles;
	vec3_t up;
	vec3_t scale;
	float target_distance;
	float sensitivity;
	float move_speed;
	float sprint_speed;
	float fov;
	float near_plane;
	float far_plane;
	alcam_mode mode;
} al_camera_desc_t;

al_camera al_camera_create(const al_camera_desc_t* desc);
void al_camera_update(al_camera* camera);

vec3_t al_camera_front(const al_camera* camera);
vec3_t al_camera_right(const al_camera* camera);
vec3_t al_camera_up(const al_camera* camera);

void al_camera_rotate(al_camera* camera, vec3_t angles);

// camera modes
alcam_mode alcam_mode_2d_grabber();
alcam_mode alcam_mode_3d_freecam();

#endif AL_CAMERA_INCLUDED
//#define AL_IMPL
#ifdef AL_IMPL 

al_camera al_camera_create(const al_camera_desc_t* desc) {
	al_camera_desc_t ref = *desc;
	if (ref.target_distance == 0.0f) ref.target_distance = 2.0f;
	if (ref.up.x == 0.0f && ref.up.y == 0.0f && ref.up.z == 0.0f) ref.up = vec3(0.0f, 1.0f, 0.0f);
	if (ref.scale.x == 0.0f && ref.scale.y == 0.0f && ref.scale.z == 0.0f) ref.scale = vec3(1.0f, 1.0f, 1.0f);
	if (ref.fov == 0.0f) ref.fov = vecmath_radians(60.0f);
	if (ref.move_speed == 0.0f) ref.move_speed = 1.0f;
	if (ref.sprint_speed == 0.0f) ref.sprint_speed = 2.0f;
	if (ref.sensitivity == 0.0f) ref.sensitivity = 300.0f;
	if (ref.far_plane == 0.0f) ref.far_plane = 64.0f;
	al_camera camera = (al_camera){
		.mode = desc->mode,
		.position = desc->position,
		.orientation = quat_identity(),
		.up = ref.up,
		.scale = ref.scale,
		.view = {
			.fov = ref.fov,
			.target_distance = ref.target_distance,
			.near_plane = ref.near_plane,
			.far_plane = ref.far_plane
		},
		.movement = {
			.move_speed = ref.move_speed,
			.sprint_speed = ref.sprint_speed,
			.sensitivity = ref.sensitivity
		}
	};
	return camera;
}

vec3_t al_camera_front(const al_camera* camera) {
	return vec3_normalize(quat_rotate_vector(vec3(0.0f, 0.0f, 1.0f), camera->orientation));
}

vec3_t al_camera_right(const al_camera* camera) {
	return vec3_normalize(vec3_cross(al_camera_front(camera), camera->up));
}

vec3_t al_camera_up(const al_camera* camera) {
	return vec3_normalize(camera->up);
}

void al_camera_rotate(al_camera* camera, vec3_t angles) {
	vec3_t front = al_camera_front(camera);
	vec3_t right = vec3_cross(front, camera->up);
	vec4_t delta_quat = quat_mul(quat_mul(quat_rotation_axis(camera->up, angles.x), quat_rotation_axis(right, angles.y)), quat_rotation_axis(front, angles.z));
	camera->orientation = quat_mul(camera->orientation, delta_quat);
	front = al_camera_front(camera);
}

void al_camera_update(al_camera* camera) {
	float dt = (float)sapp_frame_duration();
	float aspect = sapp_widthf() / sapp_heightf();
	vec3_t front = al_camera_front(camera);

	// update movement
	if (camera->mode.movement_function != 0) {
		camera->mode.movement_function(camera, dt);
	}

	// projection matrix
	mat44_t proj = { 0 };
	if (camera->mode.attributes.is_2d) {
		proj = mat44_ortho_lh(1.0f * aspect, 1.0f, camera->view.near_plane, camera->view.far_plane);
	}
	else {
		proj = mat44_perspective_fov_lh(camera->view.fov, aspect, camera->view.near_plane, camera->view.far_plane);
	}
	proj = mat44_mul_mat44(mat44_scaling(camera->scale.x, camera->scale.y, camera->scale.z), proj);

	// view matrix
	vec3_t look_at = { 0 };
	if (camera->mode.attributes.has_lookat_target) {
		vec3_t offset = vec3_add(camera->position, vec3_mulf(vec3_neg(vec3_normalize(front)), camera->view.target_distance));
		look_at = vec3_add(offset, front); // look at target position
	}
	else {
		look_at = vec3_add(camera->position, front); // look just in front of the camera
	}
	mat44_t view = mat44_look_at_lh(camera->position, look_at, camera->up);
	// final matrix
	camera->matrix = mat44_mul_mat44(view, proj);
}

//
// 2D GRABBER MOVEMENT
//

static void _al_camera_control_2d_grabber(al_camera* camera, float dt) {
	if (al_input.mouse_buttons[AL_MOUSE_RIGHT].pressed) {
		camera->position = vec3_add(camera->position, vec3_mulf(vec3_div(vec3v2f(al_input.mouse_delta, 0.0f), camera->scale), camera->movement.move_speed));
	}
	if (al_input.mouse_scroll_delta.y != 0.0f) {
		camera->scale = vec3_add(camera->scale, vec3_mulf(camera->scale, al_input.mouse_scroll_delta.y * 0.1f));
	}
}

alcam_mode alcam_mode_2d_grabber() {
	return (alcam_mode) {
		.attributes = {
			.is_2d = true,
			.has_lookat_target = false
		},
		.movement_function = _al_camera_control_2d_grabber
	};
}

//
// 3D freecam
//

static void _al_camera_movement_3d_freecam(al_camera* camera, float dt) {
	
	float speed = camera->movement.move_speed;
	if (al_input.keys[AL_KEY_LEFT_SHIFT].pressed) {
		speed = camera->movement.sprint_speed;
	}

	// get input diff (normalized)
	vec3_t input = vec3(
		(float)al_input.keys[AL_KEY_D].pressed - (float)al_input.keys[AL_KEY_A].pressed,
		(float)al_input.keys[AL_KEY_SPACE].pressed - (float)al_input.keys[AL_KEY_LEFT_CONTROL].pressed,
		(float)al_input.keys[AL_KEY_W].pressed - (float)al_input.keys[AL_KEY_S].pressed
	);
	input = vec3_normalize(input); // normalize so we dont go faster in diagonals
	input = vec3_mulf(input, speed * dt);
	// get position diff
	vec3_t front_delta = vec3_mulf(al_camera_front(camera), input.z);
	vec3_t right_delta = vec3_mulf(al_camera_right(camera), input.x);
	vec3_t up_delta = vec3_mulf(al_camera_up(camera), input.y);
	// add position update
	camera->position = vec3_add(camera->position, front_delta);
	camera->position = vec3_add(camera->position, right_delta);
	camera->position = vec3_add(camera->position, up_delta);

	// mouse look
	if (al_input.mouse_buttons[AL_MOUSE_RIGHT].pressed) {
		al_camera_rotate(camera, vec3_mulf(vec3(al_input.mouse_delta.x, -al_input.mouse_delta.y, 0.0f), camera->movement.sensitivity * dt));
		sapp_lock_mouse(true);
	}
	else {
		sapp_lock_mouse(false);
	}
}

alcam_mode alcam_mode_3d_freecam() {
	return (alcam_mode) {
		.attributes = {
			.is_2d = false,
			.has_lookat_target = false
		},
		.movement_function = _al_camera_movement_3d_freecam
	};
}

#endif AL_IMPL