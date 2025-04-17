def cuenta(palabras, palabra, low, high):
    if low > high:
        return 0

    mid = (low + high) // 2
    actual = palabras[mid]
    ascii_val = ord(actual[0])

    if palabra == actual:
        return ascii_val
    elif palabra < actual:
        return ascii_val + cuenta(palabras, palabra, low, mid - 1)
    else:
        return ascii_val + cuenta(palabras, palabra, mid + 1, high)


def main():
    path = r"C:\Users\Usuario\Downloads\TP_ACSO\TP2-x86_64\ej2\bombs\bomb30\palabras.txt"

    with open(path, "r", encoding="utf-8") as f:
        palabras = sorted([line.strip() for line in f if line.strip()])

    print(f"Total de palabras: {len(palabras)}")

    resultados = []

    for palabra in palabras:
        valor = cuenta(palabras, palabra, 0, len(palabras) - 1)
        resultados.append((valor, palabra))

    resultados.sort()

    print("\nPalabras con valor entre 401 y 798:")
    validas = 0
    for val, pal in resultados:
        if 401 < val < 799:
            print(f"{pal}: {val}")
            validas += 1
        elif val >= 799:
            break

    print(f"\nTotal de palabras válidas en ese rango: {validas}")


if __name__ == "__main__":
    main()

