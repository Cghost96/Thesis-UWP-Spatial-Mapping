with open("meshes.obj", "r") as in_file:
    with open("meshes_reversed.obj", "w") as out_file:
        lines = in_file.readlines()
        for line in lines:
            if "f" in line:
                line_split = line.split()
                out_file.write("f " + line_split[3] + " " + line_split[2] + " " + line_split[1] + "\n")
            else:
                out_file.write(line)
