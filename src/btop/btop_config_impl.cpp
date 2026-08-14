// Minimal Config namespace for btop-osd.
//
// The btop collection code reads its tunables through Config::getB/getS/getI.
// We provide only the keys that the collectors actually consult, seeded with
// the same defaults upstream btop uses. No config file is read or written.

#include "btop_config.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Config {

	std::filesystem::path conf_dir;
	std::filesystem::path conf_file;

	std::unordered_map<std::string_view, string> strings = {
		{"proc_sorting", "cpu lazy"},
		{"proc_filter", ""},
		{"cpu_sensor", "Auto"},
		{"freq_mode", "first"},
		{"cpu_core_map", ""},
		{"selected_battery", "Auto"},
		{"net_iface", ""},
		{"disks_filter", ""},
#ifdef GPU_SUPPORT
		{"shown_gpus", "nvidia amd intel apple"},
#endif
	};
	std::unordered_map<std::string_view, string> stringsTmp;

	std::unordered_map<std::string_view, bool> bools = {
		{"show_coretemp", true},
		{"show_cpu_freq", true},
		{"check_temp", true},
		{"show_battery", true},
		{"show_cpu_watts", true},
		{"show_swap", true},
		{"swap_disk", true},
		{"show_disks", true},
		{"zfs_arc_cached", true},
		{"disk_free_priv", false},
		{"use_fstab", true},
		{"only_physical", true},
		{"zfs_hide_datasets", false},
		{"show_io_stat", true},
		{"io_mode", false},
		{"io_graph_combined", false},
		{"net_auto", true},
		{"net_sync", true},
		{"proc_reversed", false},
		{"proc_per_core", false},
		{"proc_filter_kernel", false},
		{"proc_tree", false},
		{"show_detailed", false},
		{"pause_proc_list", false},
		{"proc_aggregate", false},
		{"keep_dead_proc_usage", false},
		{"disable_mouse", false},
#ifdef GPU_SUPPORT
		{"nvml_measure_pcie_speeds", true},
		{"rsmi_measure_pcie_speeds", true},
#endif
	};
	std::unordered_map<std::string_view, bool> boolsTmp;

	std::unordered_map<std::string_view, int> ints = {
		{"detailed_pid", 0},
		{"proc_tree_auto_collapse", 0},
		{"proc_selected", 0},
		{"proc_start", 0},
		{"selected_pid", 0},
		{"restore_detailed_pid", 0},
		{"selected_depth", 0},
		{"followed_pid", 0},
		{"proc_last_selected", 0},
		{"proc_followed", 0},
	};
	std::unordered_map<std::string_view, int> intsTmp;

	vector<string> available_batteries = {"Auto"};

	vector<string> current_boxes;
	vector<string> preset_list = {"cpu:0:default,mem:0:default,net:0:default,proc:0:default"};
	std::optional<int> current_preset;

	bool write_new = false;

	string validError;

} // namespace Config
