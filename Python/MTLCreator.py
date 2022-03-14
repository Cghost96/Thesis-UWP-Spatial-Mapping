NO_MTL = 1000
MTL_DIGITS = 4

with open("Mesh.mtl", "w") as file:
    INCREMENT: float = 1 / NO_MTL
    value: float = 0
    for m in range(1, NO_MTL + 1):
        file.write("newmtl Material." + str(m).zfill(MTL_DIGITS) + "\n")
        file.write(f"Kd {value:1.6f} {value:1.6f} {value:1.6f}\n\n")
        value = value + INCREMENT
