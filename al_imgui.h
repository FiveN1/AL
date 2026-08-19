#ifndef AL_IMGUI_INCLUDED
#define AL_IMGUI_INCLUDED

/*
	(13.08.2026)
	custom ImGui windows/tools

	INCLUDE BEFORE:
	- cimgui
	- sokol_time.h
	
*/


enum {
	AL_IG_MAX_PROFILER_TASKS = 8,
	AL_IG_PROFILER_HISTORY_COUNT = 32
};

/*
	alig_style

	custom imgui style

*/

void alig_set_style();

/*
	alig_profiler

	simple histogram profiler.
	you can measure how long a function takes to compute using the AL_PROFILE_FUNC macro.
	or you can log your own task using the alig_profiler_push_time() function.

	it is an immidiate mode profiler, where for every frame (since alig_profiler_clear) a measured task will be added to a stack.
	note that it is totally fine if one task is measure in one frame and not in next.

	only one instance of a profiler exists.

	inspired by: https://github.com/Raikiri/LegitProfiler
*/

typedef enum alig_profiler_colors { // colors formated as: 0xAABBGGRR
	AL_IG_COLOR_RED		= 0xFF0000FF,
	AL_IG_COLOR_GREEN	= 0xFF00FF00,
	AL_IG_COLOR_BLUE	= 0xFFFF0000,
	AL_IG_COLOR_YELLOW	= 0xFF00FFFF,
	AL_IG_COLOR_CYAN	= 0xFFFFFF00,
	AL_IG_COLOR_PINK	= 0xFFFF00FF,
	AL_IG_COLOR_WHITE	= 0xFFFFFFFF
} alig_profiler_colors;

void alig_profiler_clear();
int alig_profiler_push_time(float time_ms, const char* name, uint32_t color);
void alig_profiler_draw(const char* name, ImVec2_c frame_size, float scale_min, float scale_max);
bool* alig_profiler_open();
void alig_profiler_show_remaining_time(bool draw);
#define AL_PROFILE_SCOPE(scope, name, color) { uint64_t t0 = stm_now(); scope; alig_profiler_push_time((float)stm_ms(stm_diff(stm_now(), t0)), (name), (color)); } 



#endif AL_IMGUI_INCLUDED
#ifdef AL_IMPL

//
// IMPLEMENTATION
//

//
// STYLE
//

void alig_set_style() {
	ImVec4_c* colors = igGetStyle()->Colors;
	colors[ImGuiCol_WindowBg] = (ImVec4_c){ 0.1f, 0.105f, 0.11f, 1.0f };
	// headers
	colors[ImGuiCol_Header] = (ImVec4_c){ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_HeaderHovered] = (ImVec4_c){ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_HeaderActive] = (ImVec4_c){ 0.15f, 0.1505f, 0.151f, 1.0f };
	// buttons
	colors[ImGuiCol_Button] = (ImVec4_c){ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_ButtonHovered] = (ImVec4_c){ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_ButtonActive] = (ImVec4_c){ 0.15f, 0.1505f, 0.151f, 1.0f };
	// frame BG
	colors[ImGuiCol_FrameBg] = (ImVec4_c){ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = (ImVec4_c){ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = (ImVec4_c){ 0.15f, 0.1505f, 0.151f, 1.0f };
	// tabs
	colors[ImGuiCol_Tab] = (ImVec4_c){ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabHovered] = (ImVec4_c){ 0.38f, 0.3805f, 0.381f, 1.0f };
	colors[ImGuiCol_TabSelected] = (ImVec4_c){ 0.28f, 0.2805f, 0.281f, 1.0f };
	colors[ImGuiCol_TabDimmed] = (ImVec4_c){ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabDimmedSelected] = (ImVec4_c){ 0.2f, 0.205f, 0.21f, 1.0f };
	// title
	colors[ImGuiCol_TitleBg] = (ImVec4_c){ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = (ImVec4_c){ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = (ImVec4_c){ 0.15f, 0.1505f, 0.151f, 1.0f };
}

//
// PROFILER
//

typedef struct _alig_profiler_task {
	float time;
	const char* name;
	uint32_t color;
} _alig_profiler_task;

typedef struct _alig_profiler_task_stack {
	_alig_profiler_task tasks[AL_IG_MAX_PROFILER_TASKS];
	int count;
} _alig_profiler_task_stack;

typedef struct _alig_profiler {
	_alig_profiler_task_stack task_stack;
	_alig_profiler_task_stack finished_task_stack; // task stack with all measured tasks. the reason this exsits is because some task may be measured after drawing and before clearing and will not be plotted.
	float finished_frame_time;
	bool window_open;
	bool dont_draw_remaining_time;
} _alig_profiler;

static _alig_profiler _profiler;

void alig_profiler_clear() {
	_profiler.finished_task_stack = _profiler.task_stack; // measuring has finished, store the results for drawing. (note that finished_task_stack is one frame behind)
	_profiler.task_stack.count = 0;
	static uint64_t t0 = 0;
	_profiler.finished_frame_time = (float)stm_ms(stm_diff(stm_now(), t0)); // also measure time since last alig_profiler_clear()
	t0 = stm_now();
}

int alig_profiler_push_time(float time_ms, const char* name, uint32_t color) {
	if (_profiler.task_stack.count >= AL_IG_MAX_PROFILER_TASKS) {
		printf("ERROR: alig_profiler_push_time reached maximim amount of tasks! make sure to clear all tasks every frame!\n");
		return;
	}
	int task_index = _profiler.task_stack.count++;
	_profiler.task_stack.tasks[task_index] = (_alig_profiler_task){
		.time = time_ms,
		.name = name,
		.color = color
	};
	return task_index;
}

typedef struct alig_histogram_desc {
	const char* name;
	const char* overlay_text;
	const ImVec2_c frame_size;
	_alig_profiler_task_stack* tasks_history;
	int history_count;
	int history_offset;
	float scale_min;
	float scale_max;
} alig_histogram_desc;

// get dimensions in the y axis of a plot segment/task in normalised coordinates: 0.0f - 1.0f
ImVec2_c _alig_calc_plot_dim(float base_y, float time_ms, float scale_min, float scale_max) {
	float plot_height = ((time_ms) / (scale_max - scale_min)); // task height in graph
	ImVec2_c plot_y = (ImVec2_c){ base_y, base_y + plot_height };
	return igImClamp(plot_y, (ImVec2_c) { 0.0f, 0.0f }, (ImVec2_c) { 1.0f, 1.0f }); // clamp in this range. plot_y will be later scaled to inner size
}

void _alig_profiler_draw_histogram(alig_histogram_desc* desc) {

	// imgui data
	const ImGuiStyle* style = igGetStyle();
	const ImGuiWindow* window = igGetCurrentWindow();
	const ImGuiID id = ImGuiWindow_GetID_Str(window, desc->name, NULL);
	const ImDrawList* draw_list = igGetWindowDrawList();
	const ImU32 frame_bg_color = 0XFF000000; // igGetColorU32_Col(ImGuiCol_FrameBg, 1.0f)
	const float marker_width = 32.0f;
	_alig_profiler_task_stack* last_task_stack = &desc->tasks_history[(desc->history_offset + desc->history_count - 1) % desc->history_count];

	// wanky way of calculating formated text width
	float max_label_width = 0.0f;
	for (int i = 0; i < last_task_stack->count; i++) {
		float label_width = igCalcTextSize(last_task_stack->tasks[i].name, NULL, true, -1.0f).x;
		max_label_width = fmaxf(max_label_width, label_width);
	}
	max_label_width += igCalcTextSize("[0.000ms] ", NULL, true, -1.0f).x;

	// rects
	const ImRect_c graph_bb = {
		.Min = window->DC.CursorPos,
		.Max = {window->DC.CursorPos.x + desc->frame_size.x, window->DC.CursorPos.y + desc->frame_size.y}
	};
	const ImRect_c inner_bb = {
		.Min = { graph_bb.Min.x + style->FramePadding.x, graph_bb.Min.y + style->FramePadding.y },
		.Max = { graph_bb.Max.x - style->FramePadding.x, graph_bb.Max.y - style->FramePadding.y }
	};
	const ImRect_c total_bb = {
		.Min = graph_bb.Min,
		.Max = {
			graph_bb.Max.x + style->ItemInnerSpacing.x * 2.0f + marker_width + max_label_width,//window->ParentWorkRect.Max.x - (float)(int)(window->WindowPadding.x * 0.5f), // clamp to window border
			graph_bb.Max.y
		}
	};

	// add histogram item
	igItemSize_Rect(total_bb, style->FramePadding.y);
	if (!igItemAdd(total_bb, id, &graph_bb, ImGuiItemFlags_NoNav)) {
		return;
	}

	// draw frame
	igRenderFrame(total_bb.Min, total_bb.Max, frame_bg_color, 0.0f, style->FrameRounding);

	// draw plots
	const float inner_width = inner_bb.Max.x - inner_bb.Min.x;
	const float inner_height = inner_bb.Max.y - inner_bb.Min.y;
	const float plot_width = inner_width / desc->history_count;
	const float plot_base_height = (-desc->scale_min / (desc->scale_max - desc->scale_min));

	for (int i = 0; i < desc->history_count; i++) { // for each history
		_alig_profiler_task_stack* task_stack = &desc->tasks_history[(i + desc->history_offset) % desc->history_count];

		ImRect_c plot_base = {
			.Min = {inner_bb.Min.x + (float)i * plot_width, inner_bb.Max.y},
			.Max = {inner_bb.Min.x + (float)i * plot_width + (plot_width - 1.0f), inner_bb.Max.y}
		};
		float accumulated_height = plot_base_height;

		// plot each task in task stack
		for (int j = 0; j < task_stack->count; j++) { // for each task
			_alig_profiler_task* task = &task_stack->tasks[j].time;

			// calc task graph size
			ImVec2_c plot_y = _alig_calc_plot_dim(accumulated_height, task->time, desc->scale_min, desc->scale_max);
			accumulated_height = plot_y.y; // update accumulated height
			// graph task
			ImRect_c plot_bb = {
				.Min = { plot_base.Min.x, plot_base.Min.y - plot_y.x * inner_height },
				.Max = { plot_base.Max.x, plot_base.Max.y - plot_y.y * inner_height }
			};
			ImDrawList_AddRectFilled(draw_list, plot_bb.Min, plot_bb.Max, (ImU32)task->color, 0.0f, 0);
		}
	}

	// draw names & markers
	const float font_size = style->FontSizeBase;
	float y_offset = 0.0f;
	float accumulated_height = plot_base_height;
	for (int i = 0; i < last_task_stack->count; i++) {
		_alig_profiler_task* task = &last_task_stack->tasks[i].time;
		y_offset += font_size;
		// draw text
		char buff[128];
		snprintf(buff, sizeof(buff) / sizeof(*buff), "[%0.3fms] %s", task->time, task->name);
		igPushStyleColor_U32(ImGuiCol_Text, (ImU32)task->color);
		igRenderText((ImVec2) { graph_bb.Max.x + style->ItemInnerSpacing.x + marker_width, inner_bb.Max.y - y_offset }, buff, NULL, true);
		igPopStyleColor(1);
		// get plot y dimensions
		ImVec2_c plot_y = _alig_calc_plot_dim(accumulated_height, task->time, desc->scale_min, desc->scale_max);
		accumulated_height = plot_y.y;
		// draw marker
		ImVec2_c vertices[4] = {
			{ graph_bb.Max.x + marker_width, inner_bb.Max.y - y_offset + 2.0f }, // vertices nex to label
			{ graph_bb.Max.x + marker_width, inner_bb.Max.y - y_offset + font_size },
			{ graph_bb.Max.x + style->ItemInnerSpacing.x, inner_bb.Max.y - plot_y.x * inner_height }, // vertices next to plot
			{ graph_bb.Max.x + style->ItemInnerSpacing.x, inner_bb.Max.y - plot_y.y * inner_height }
		};
		ImDrawList_AddConvexPolyFilled(draw_list, vertices, 4, (ImU32)task->color);
	}

	// overlay text
	if (desc->overlay_text) {
		igRenderTextClipped((ImVec2) { total_bb.Min.x, total_bb.Min.y + style->FramePadding.y }, total_bb.Max, desc->overlay_text, NULL, NULL, (ImVec2) { 0.5f, 0.0f }, NULL);
	}
}

void alig_profiler_draw(const char* name, ImVec2_c frame_size, float scale_min, float scale_max) {
	if (_profiler.window_open) {
		static char win_title[128] = "[0.0ms 0fps] Profiler###alig_profiler_window";
		igBegin(win_title, &_profiler.window_open, 0);

		// profiler history
		static _alig_profiler_task_stack tasks_history[AL_IG_PROFILER_HISTORY_COUNT];
		static int histogram_offset = 0;
		static float refersh_time = 0.0f;

		// hisotry of frame times
		static float frame_times[AL_IG_PROFILER_HISTORY_COUNT];
		static float average = 0.0f;
		static float maximum = 0.0f;

		// refresh
		refersh_time += _profiler.finished_frame_time;
		if (refersh_time > (1.0f / 24.0f) * 1000.0f) { // 24hz update rate
			// update frame times, avg and max stats
			frame_times[histogram_offset] = _profiler.finished_frame_time;
			maximum = 0.0f;
			average = 0.0f;
			for (int i = 0; i < AL_IG_PROFILER_HISTORY_COUNT; i++) {
				average += frame_times[i];
				maximum = fmaxf(maximum, frame_times[i]);
			}
			average /= (float)AL_IG_PROFILER_HISTORY_COUNT;

			// add remaining time
			if (!_profiler.dont_draw_remaining_time && _profiler.finished_task_stack.count < AL_IG_MAX_PROFILER_TASKS) {
				float measured_time = 0.0f;
				for (int i = 0; i < _profiler.finished_task_stack.count; i++) {
					measured_time += _profiler.finished_task_stack.tasks[i].time;
				}
				int task_index = _profiler.finished_task_stack.count++;
				_profiler.finished_task_stack.tasks[task_index] = (_alig_profiler_task){
					.time = _profiler.finished_frame_time - measured_time,
					.color = 0xFF333333,
					.name = "other"
				};
			}

			// update histogram
			tasks_history[histogram_offset] = _profiler.finished_task_stack;
			histogram_offset = (histogram_offset + 1) % AL_IG_PROFILER_HISTORY_COUNT;
			refersh_time = 0.0f;
			// update win_title
			snprintf(win_title, sizeof(win_title) / sizeof(*win_title), "[%0.3fms %0.0ffps] Profiler###alig_profiler_window", _profiler.finished_frame_time, 1000.0f / _profiler.finished_frame_time);
		}

		// update overlay
		char overlay[32];
		snprintf(overlay, 32, "avg %0.3fms\nmax %0.3fms", average, maximum);

		// draw histogram
		_alig_profiler_draw_histogram(&(alig_histogram_desc) {
			.name = name,
				.overlay_text = overlay,
				.frame_size = frame_size,
				.tasks_history = tasks_history,
				.history_count = AL_IG_PROFILER_HISTORY_COUNT,
				.history_offset = histogram_offset,
			.scale_min = scale_min,
			.scale_max = scale_max,
		});
		
		igEnd();
	}
}

bool* alig_profiler_open() {
	return &_profiler.window_open;
}

void alig_profiler_show_remaining_time(bool draw) {
	_profiler.dont_draw_remaining_time = !draw;
}

#endif AL_IMPL
