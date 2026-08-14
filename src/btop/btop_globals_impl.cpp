// Definitions of the btop globals (Global / Runner / per-box layout state) and
// the handful of free functions that the collection code and tools need.
//
// These values are only used to keep the copied btop collectors linkable and
// are never consulted for drawing, so they default to harmless values. The
// `width` values must be non-zero so the collectors keep a bounded history
// instead of trimming every sample.

#include "btop_shared.hpp"
#include "btop_tools.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

//? -------------------------------------------------- Global -------------------------------------------------

namespace Global {
	const vector<array<string, 2>> Banner_src;
	const string Version = "btop-osd";
	atomic<bool> quitting (false);
	string exit_error_msg;
	atomic<bool> thread_exception (false);
	string banner;
	atomic<bool> resized (false);
	string overlay;
	string clock;
	uid_t real_uid = getuid();
	uid_t set_uid = geteuid();
	atomic<bool> init_conf (false);
}

//? -------------------------------------------------- Runner -------------------------------------------------

namespace Runner {
	Tools::atomic_waiting_lock active;
	atomic<bool> reading (false);
	atomic<bool> stopping (false);
	atomic<bool> redraw (false);
	atomic<bool> coreNum_reset (false);
	bool pause_output = false;
	string debug_bg;
}

//? -------------------------------------------------- Free functions -------------------------------------------------

void term_resize(bool /*force*/) {}
void banner_gen() {}
void clean_quit(int) {}

//? Fx::reset is normally defined by btop_theme.cpp (rendering code we do not
//? vendor); provide it here for the tools that reference it.
namespace Fx {
	string reset = reset_base;
}

//? -------------------------------------------------- Layout state (Cpu) -------------------------------------------------

namespace Cpu {
	string box;
	int x = 0, y = 0, width = 160, height = 0, min_width = 0, min_height = 0;
	bool shown = false, redraw = false;
}

//? -------------------------------------------------- Layout state (Mem) -------------------------------------------------

namespace Mem {
	string box;
	int x = 0, y = 0, width = 160, height = 0, min_width = 0, min_height = 0;
	bool shown = false, redraw = false;
}

//? -------------------------------------------------- Layout state (Net) -------------------------------------------------

namespace Net {
	string box;
	int x = 0, y = 0, width = 160, height = 0, min_width = 0, min_height = 0;
	bool shown = false, redraw = false;
}

//? -------------------------------------------------- Layout state (Proc) -------------------------------------------------

namespace Proc {
	string box;
	int x = 0, y = 0, width = 160, height = 0, min_width = 0, min_height = 0;
	bool shown = false, redraw = false;
	int select_max = 0;
	atomic<int> detailed_pid (0);
	int selected_pid = 0, start = 0, selected = 0;
	int scroll_pos = 0;
	string selected_name;
	atomic<bool> resized (false);
}

//? -------------------------------------------------- Layout state (Gpu) -------------------------------------------------

namespace Gpu {
	vector<string> box;
	int width = 160, total_height = 0, min_width = 0, min_height = 0;
	vector<int> x_vec, y_vec;
	vector<bool> redraw;
	int shown = 0;
	int count = 0;
	vector<int> shown_panels;
}
