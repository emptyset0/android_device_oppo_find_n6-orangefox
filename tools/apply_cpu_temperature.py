"""Select a real CPU thermal zone in the N6 port, without hard-coding its index."""
from pathlib import Path
import sys


p = Path(sys.argv[1]) / 'bootable/recovery/data.cpp'
source = p.read_text()
old_match = '      if (type.compare(0, 4, "cpu-") != 0)'
new_match = '''      unsigned cluster, core, sensor;
      char trailing;
      if (sscanf(type.c_str(), "cpu-%u-%u-%u%c", &cluster, &core, &sensor, &trailing) != 3)'''
if 'ReadRecoveryCpuTemperature' in source:
    # The first preview accepted cpu-hw-trip-*, which exposes the hardware
    # trip threshold (105 C), not the current CPU temperature.
    if old_match in source:
        assert source.count(old_match) == 1
        p.write_text(source.replace(old_match, new_match))
    else:
        assert new_match in source
    sys.exit(0)

include = '#include <fstream>\n'
assert source.count(include) == 1
source = source.replace(include, include + '#include <dirent.h>\n')

start = source.index('  string cpu_temp_file;\n', source.index('#ifdef TW_NO_CPU_TEMP'))
end = source.index('\n#endif\n#ifdef TW_CUSTOM_POWER_BUTTON', start)
source = (source[:start] + '  // CPU thermal zones may finish probing after the early defaults.\n'
          '  mConst.SetValue("tw_no_cpu_temp", "0");' + source[end:])

anchor = 'extern bool datamedia;\n'
assert source.count(anchor) == 1
source = source.replace(anchor, anchor + r'''

namespace {
long ReadRecoveryCpuTemperature(const std::string& path) {
  std::ifstream file(path);
  long milli_celsius = 0;
  // A powered-down thermal zone can return 0 or fail to supply a sample.
  return (file >> milli_celsius) && milli_celsius >= 1000 && milli_celsius < 150000
      ? milli_celsius : 0;
}

long GetRecoveryCpuTemperature(std::string& preferred) {
#ifdef TW_CUSTOM_CPU_TEMP_PATH
  if (const long temp = ReadRecoveryCpuTemperature(EXPAND(TW_CUSTOM_CPU_TEMP_PATH)))
    return temp;
#endif
  if (!preferred.empty()) {
    if (const long temp = ReadRecoveryCpuTemperature(preferred))
      return temp;
    preferred.clear();
  }
  const std::string root = "/sys/class/thermal";
  if (DIR* dir = opendir(root.c_str())) {
    long best = 0;
    while (const auto* entry = readdir(dir)) {
      const std::string name(entry->d_name);
      if (name.compare(0, 12, "thermal_zone") != 0)
        continue;
      const std::string zone = root + "/" + name;
      std::string type;
      std::ifstream(zone + "/type") >> type;
      // N6's direct CPU sensors have cpu-N-N-N labels, independent of zone number.
      if (type.compare(0, 4, "cpu-") != 0)
        continue;
      const long temp = ReadRecoveryCpuTemperature(zone + "/temp");
      if (temp > best) {
        best = temp;
        preferred = zone + "/temp";
      }
    }
    closedir(dir);
    if (best)
      return best;
  }
  return ReadRecoveryCpuTemperature(root + "/thermal_zone0/temp");
}
}  // namespace
''')

start = source.index('\t\tstring cpu_temp_file;\n', source.index('else if (varName == "tw_cpu_temp")'))
end = source.index('\n\t\treturn 0;', start)
source = source[:start] + '''		static std::string cpu_path;
		static std::string cpu_display = "--";
		static time_t next_sample = 0;
		struct timespec now{};
		clock_gettime(CLOCK_MONOTONIC, &now);
		if (now.tv_sec >= next_sample) {
			const long temp = GetRecoveryCpuTemperature(cpu_path);
			cpu_display = temp ? std::to_string(temp / 1000) : "--";
			next_sample = now.tv_sec + 5;
		}
		value = cpu_display;''' + source[end:]
assert source.count(old_match) == 1
p.write_text(source.replace(old_match, new_match))
