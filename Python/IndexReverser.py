import re

with open("test.obj", "r") as in_file:
    with open("meshes_reversed.obj", "w") as out_file:
        lines = in_file.readlines()
        for line in lines:
            if line.find('f ') == 0:
                line_split = re.split(' |//|\n', line)
                out_file.write('f '
                               + line_split[5] + '//' + line_split[5] + ' '
                               + line_split[3] + '//' + line_split[3] + ' '
                               + line_split[1] + '//' + line_split[1] + '\n')
            else:
                out_file.write(line)
