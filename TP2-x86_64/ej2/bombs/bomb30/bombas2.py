import random

def generar_input_valido():
    # Generamos dos números aleatorios, uno positivo y otro negativo.
    num1 = random.randint(1, 10)
    num2 = random.randint(-10, -1)

    # Realizamos la operación XOR entre los dos números
    xor_result = num1 ^ num2

    # Desplazamos el resultado a la derecha (equivalente a dividir por 2)
    shifted_result = xor_result >> 1

    # El tercer número será el resultado del desplazamiento
    num3 = shifted_result

    return num1, num2, num3

# Generamos y mostramos 5 ejemplos válidos
for _ in range(5):
    num1, num2, num3 = generar_input_valido()
    print(f"Entrada válida: {num1} {num2} {num3}")
