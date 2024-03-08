import logging
import sys
from offset_name_map import names

if(len(sys.argv) != 2):
    exit('Usage: python3 offset_parser.py <input_file_name>')

offsets = dict()
file_name = sys.argv[1]

offsets_file = open('offsets.h', 'w')

with open(file_name, 'r') as file:
    last_header = ''
    for line in file.readlines():
        if line[0] == '[' and line[-2] == ']':
            last_header = line[1:-2]
            continue
        elif '=' not in line:
            continue
        offset_name, offset_value = line.split('=', 2)
        try:
            num = int(offset_value, 16)
            if offset_name == 'm_vecAbsOrigin' and last_header != 'DataMap.CBaseViewModel':
                continue
            elif offset_name not in offsets:
                offsets[offset_name] = offset_value.strip()
        except ValueError:
            logging.debug(f'{offset_name}: {offset_value} is not a hex value')

offsets_file.write("#define ORIGIN 1\n#define STEAM 2\n#define VERSION STEAM\n#if VERSION == STEAM\n")
for key in names.keys():
    if (key in offsets.keys()):
        offsets_file.write(f'#define {names[key]}\t\t{offsets[key]}\n')
    else:
        logging.warning(f'{key} was not found in offsets paste')
offsets_file.write(f'#define OFFSET_BONES\t\t{offsets["m_nForceBone"]} + 0x48\n')
offsets_file.write(f'#define OFFSET_VIEWANGLES\t\t{offsets["m_ammoPoolCapacity"]} - 0x14\n')
offsets_file.write(f'#define OFFSET_BREATH_ANGLES\t\tOFFSET_VIEWANGLES - 0x10\n')
offsets_file.write(f'#define OFFSET_ZOOM_FOV\t\tOFFSET_PLAYER_DATA + OFFSET_CURRENT_ZOOM\n')
# offsets_file.write(f'#define OFFSET_VISIBLE_TIME\t\t0x1aa0\n')
offsets_file.write(f'#define OFFSET_LOCAL_ENT\t\t{offsets["LocalPlayer"]}\n')
# offsets_file.write(f'#define OFFSET_BULLET_SPEED\t\tOFFSET_VISIBLE_TIME + 0x4cc\n')
# offsets_file.write(f'#define OFFSET_BULLET_SCALE\t\tOFFSET_VISIBLE_TIME + 0x4d4\n')
offsets_file.write(f'#endif\n')
