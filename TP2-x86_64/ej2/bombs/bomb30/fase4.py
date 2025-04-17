def candidates_for(index):
    return [chr(x) for x in range(32, 127) if (x & 0xF) == index]

# La palabra que querés lograr es "colina"
# Estos son los índices que producen esas letras en la tabla array.0
indices = {
    'c': 0x3,
    'o': 0x8,
    'l': 0xf,
    'i': 0x6,
    'n': 0xa,
    'a': 0x5,
}

# Mostrar posibles caracteres para cada letra deseada
for letter, index in indices.items():
    options = candidates_for(index)
    print(f"{letter.upper()} (índice {hex(index)}): {' '.join(options)}")
