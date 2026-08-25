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

/*
	alig_console

	imgui console for logging messages and calling commands.

*/

// command callback. function arguments: (int arg_count. char* arguments[]). arguments is of size arg_count.
typedef void(*alig_console_command_callback)(int, char*[]);

void alig_console_init(bool is_open); // will init console with some basic commands
void alig_console_set_log_color(uint32_t color); // set color for the next log
void alig_console_log(const char* fmt, ...);
void alig_console_log_unformatted(const char* message);
void alig_console_exec(const char* command);
void alig_console_add_command(const char* name, alig_console_command_callback callback);
void alig_console_clear();
void alig_console_draw(const char* name);

#endif AL_IMGUI_INCLUDED
#define AL_IMPL
#ifdef AL_IMPL

//
// IMPLEMENTATION
//

#define AL_ARRAY_SIZE(_arr) ((int)(sizeof(_arr) / sizeof(*(_arr))))

//
// STYLE
// >>style

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
//>>profiler

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

static _alig_profiler _profiler; // private instance of a profiler

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

typedef struct _alig_histogram_desc {
	const char* name;
	const char* overlay_text;
	const ImVec2_c frame_size;
	_alig_profiler_task_stack* tasks_history;
	int history_count;
	int history_offset;
	float scale_min;
	float scale_max;
} _alig_histogram_desc;

// get dimensions in the y axis of a plot segment/task in normalised coordinates: 0.0f - 1.0f
static ImVec2_c _alig_calc_plot_dim(float base_y, float time_ms, float scale_min, float scale_max) {
	float plot_height = ((time_ms) / (scale_max - scale_min)); // task height in graph
	ImVec2_c plot_y = (ImVec2_c){ base_y, base_y + plot_height };
	return igImClamp(plot_y, (ImVec2_c) { 0.0f, 0.0f }, (ImVec2_c) { 1.0f, 1.0f }); // clamp in this range. plot_y will be later scaled to inner size
}

static void _alig_profiler_draw_histogram(_alig_histogram_desc* desc) {

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
		_alig_profiler_draw_histogram(&(_alig_histogram_desc) {
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

//
// CONSOLE
// >>console

enum {
	AL_IG_MAX_LOG_ITEMS = 256,
	AL_IG_MAX_LOG_CHARACTERS = 4096,
	AL_IG_MAX_COMMANDS = 256,
	AL_IG_MAX_COMMAND_ARGUMENTS = 64
};

typedef struct _alig_console_log_item {
	uint32_t color;	// color of log item formated as 0xAABBGGRR
	int log_index;	// index to log in the log_buff
} _alig_console_log_item;

typedef struct _alig_console_log_items {
	_alig_console_log_item data[AL_IG_MAX_LOG_ITEMS];
	int count;
} _alig_console_log_items;

typedef struct _alig_console_logs {
	char data[AL_IG_MAX_LOG_CHARACTERS];	// buffer with all of the messages
	int count;								// used characters count
} _alig_console_logs;

typedef struct _alig_console_command {
	char name[256];
	alig_console_command_callback callback; // command callback called on exec
} _alig_console_command;

typedef struct _alig_console_commands {
	_alig_console_command data[AL_IG_MAX_COMMANDS];
	int count;
} _alig_console_commands;

typedef struct _alig_console_command_arguments {
	char* pointers[AL_IG_MAX_COMMAND_ARGUMENTS];	// pointers to arguments in data buffer (this is the char* args[])
	char data[AL_IG_MAX_COMMAND_ARGUMENTS][256];	// argument buffer
	int count;										// argument count (this is the int arg_count)
} _alig_console_command_arguments;

typedef struct _alig_console {
	_alig_console_log_items log_items;				// data about seperate text instances, i.e. colors & ptrs to strings in log buffer, of seperate text instances
	_alig_console_logs logs;						// log item data, all of the strings are stored here
	_alig_console_commands commands;				// data about added commands
	_alig_console_command_arguments command_args;	// command argument data for when calling exec()
	char input_buff[256];							// text input section buffer
	uint32_t next_log_color;
	bool scroll_to_bottom;
	bool is_open;				
} _alig_console;

static _alig_console _console; // private instance

static void _alig_console_help_command(int arg_count, char* args[]) {
	alig_console_log_unformatted("list of available commands:");
	for (int i = 0; i < _console.commands.count; i++) {
		_alig_console_command* command = &_console.commands.data[i];
		char buff[256];
		snprintf(buff, sizeof(buff), "- %s", command->name);
		alig_console_log_unformatted(buff);
	}
}

static void _alig_console_clear_command(int arg_count, char* args[]) {
	alig_console_clear();
}

static void _alig_console_stats_command(int arg_count, char* args[]) {
	alig_console_log_unformatted("console stats:");
	alig_console_log("- log items: %i/%i", _console.log_items.count, AL_ARRAY_SIZE(_console.log_items.data));
	alig_console_log("- log buffer: %i/%i", _console.logs.count, AL_ARRAY_SIZE(_console.logs.data));
	alig_console_log("- commands: %i/%i", _console.commands.count, AL_ARRAY_SIZE(_console.commands.data));
}

void alig_console_init(bool is_open) {
	_console.is_open = is_open;
	_console.next_log_color = AL_IG_COLOR_WHITE;
	// add default log
	alig_console_log_unformatted("type 'help' for a list of available commands");
	// add default commands
	alig_console_add_command("help", _alig_console_help_command);
	alig_console_add_command("clear", _alig_console_clear_command);
	alig_console_add_command("stats", _alig_console_stats_command);
}

void alig_console_set_log_color(uint32_t color) { // i like this split between the log and color function
	_console.next_log_color = color;
}

void alig_console_log(const char* fmt, ...) {
	// from: https://stackoverflow.com/questions/66094905/how-to-pass-a-formatted-string-as-a-single-argument-in-c
	// determine required buffer size 
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	if (len < 0) return;
	// format message
	char message[256];
	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);
	// log
	alig_console_log_unformatted(message);
}

void alig_console_log_unformatted(const char* message) {
	int log_items_capacity = AL_ARRAY_SIZE(_console.log_items.data);
	int log_buff_capacity = AL_ARRAY_SIZE(_console.logs.data);
	// check if reached maximum capacity of any buffer, if yes then clear the whole console
	if (_console.log_items.count >= log_items_capacity || _console.logs.count >= log_buff_capacity) {
		alig_console_clear();
	}
	// add message (will be trimmed if it wount fint into the buffer)
	int message_length = snprintf(&_console.logs.data[_console.logs.count], (size_t)(log_buff_capacity - _console.logs.count), "%s", message);
	if (message_length == 0) return; // return if empty message (wont add any log items)
	// add log item
	_alig_console_log_item* log_item = &_console.log_items.data[_console.log_items.count++];
	*log_item = (_alig_console_log_item){
		.color = _console.next_log_color,
		.log_index = _console.logs.count
	};
	// increment log buff index
	_console.logs.count += message_length + 1; // +1 for the null terminator
	// set next color to default
	_console.next_log_color = AL_IG_COLOR_WHITE;
}

static void _alig_console_split_command(const char* command, int max_arg_count, int max_arg_size, char* out_arg_buff, int* out_arg_count, char* out_arg_ptrs[]) {
	// loop until no arguments separated by spaces
	char* arg_begin = command;
	for (; (*out_arg_count) < max_arg_count; (*out_arg_count)++) {
		// get argument size
		char* arg_end = strchr(arg_begin, ' ');
		size_t arg_size = 0;
		if (arg_end == NULL) arg_size = strnlen(arg_begin, max_arg_size);
		else arg_size = (size_t)(arg_end - arg_begin);
		// check before proceeding to copy the argument
		if (arg_size == 0 || arg_size >= max_arg_size) break;
		// copy to arg buffer
		char* arg_buff = &out_arg_buff[max_arg_size * (*out_arg_count)];
		memcpy(arg_buff, arg_begin, arg_size);
		arg_buff[arg_size] = '\0';
		// add pointer
		out_arg_ptrs[*out_arg_count] = arg_buff;
		// increment (+1 for space) NOTE: commands with multiple spaces cause problems, or with spaces at the begining!!
		arg_begin += arg_size + 1;
	}
}

void alig_console_exec(const char* command) {
	// reset argument count
	_console.command_args.count = 0;
	// split command into arguments separated by spaces
	_alig_console_split_command(
		command, 
		AL_ARRAY_SIZE(_console.command_args.data), 
		AL_ARRAY_SIZE(_console.command_args.data[0]), 
		_console.command_args.data, 
		&_console.command_args.count, 
		_console.command_args.pointers
	);
	// check if command was valid (must have at least one argument)
	if (_console.command_args.count == 0) {
		alig_console_set_log_color(AL_IG_COLOR_YELLOW);
		alig_console_log_unformatted("invalid command");
		return;
	}
	// find command. NOTE: it would be great if the commands were sorted alphabetically so we could find them in log time
	const char* called_command_name = _console.command_args.pointers[0]; // command name is always the first argument
	for (int i = 0; i < _console.commands.count; i++) {
		_alig_console_command* command = &_console.commands.data[i];
		if (strcmp(command->name, called_command_name) == 0) {
			command->callback(_console.command_args.count, _console.command_args.pointers); // call command
			// scroll to bottom
			_console.scroll_to_bottom = true;
			return;
		}
	}
	// if command not found
	alig_console_set_log_color(AL_IG_COLOR_YELLOW);
	alig_console_log_unformatted("command not found");
}

void alig_console_add_command(const char* name, alig_console_command_callback callback) {
	if (_console.commands.count >= AL_ARRAY_SIZE(_console.commands.data)) return;
	_alig_console_command* command = &_console.commands.data[_console.commands.count++];
	snprintf(command->name, (size_t)AL_ARRAY_SIZE(command->name), "%s", name);
	command->callback = callback;
}

void alig_console_clear() {
	_console.log_items.count = 0;
	_console.logs.count = 0;
}

void alig_console_draw(const char* name) {
	if (!_console.is_open) {
		return;
	}

	igSetNextWindowSize((ImVec2_c) { 480, 360 }, ImGuiCond_FirstUseEver);
	igBegin(name, &_console.is_open, 0);

	// display logs
	const float footer_height_to_reserve = igGetStyle()->ItemSpacing.y + igGetFrameHeightWithSpacing();
	if (igBeginChild_Str("ScrollingRegion", (ImVec2_c) { 0, -footer_height_to_reserve }, ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
		igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2_c) { 4, 1 }); // Tighten spacing
		for (int i = 0; i < _console.log_items.count; i++) {
			_alig_console_log_item* log_item = &_console.log_items.data[i];
			// render log item
			igPushStyleColor_U32(ImGuiCol_Text, log_item->color);
			igTextUnformatted(&_console.logs.data[log_item->log_index], NULL);
			igPopStyleColor(1);
		}
		// scroll to bottom
		if (_console.scroll_to_bottom || (igGetScrollY() >= igGetScrollMaxY())) {
			igSetScrollHereY(1.0f);
		}
		_console.scroll_to_bottom = false;

		igPopStyleVar(1);
	}
	igEndChild();
	igSeparator();

	// input section
	bool reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
	if (igInputText("Input", _console.input_buff, sizeof(_console.input_buff) / sizeof(*_console.input_buff), input_text_flags, NULL, NULL)) {
		alig_console_set_log_color(AL_IG_COLOR_CYAN);
		alig_console_log_unformatted(_console.input_buff);
		alig_console_exec(_console.input_buff);
		_console.input_buff[0] = '\0'; // clear input buffer
		reclaim_focus = true;
	}
	igSetItemDefaultFocus();
	if (reclaim_focus) {
		igSetKeyboardFocusHere(-1); // auto focus previous widget
	}
		
	igEnd();
}

#endif AL_IMPL
