; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares que pueden llegar a necesitar:
extern malloc
extern free
extern strlen
extern strcpy
extern str_concat

string_proc_list_create_asm:
    push    rbp
    mov     rbp, rsp

    ; Llamamos a malloc(16)
    mov     edi, 16
    call    malloc

    ; RAX contiene el puntero devuelto. Inicializamos los campos.
    ; *(rax) = 0
    mov     QWORD [rax], 0
    ; *(rax + 8) = 0
    mov     QWORD [rax + 8], 0

    ; RAX ya tiene el puntero que devolveremos
    pop     rbp
    ret



string_proc_node_create_asm:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32                   ; Espacio para almacenar dos valores

    ; Guardar argumentos
    mov     eax, edi
    mov     QWORD [rbp-32], rsi       ; Guardar puntero a data
    mov     BYTE  [rbp-24], al        ; Guardar carácter

    ; malloc(32)
    mov     edi, 32
    call    malloc

    ; Movemos el puntero de malloc a rax
    mov     rdx, rax

    ; Inicializar next y prev en 0
    mov     QWORD [rdx], 0
    mov     QWORD [rdx+8], 0

    ; Escribir el carácter en offset +16
    movzx   eax, BYTE [rbp-24]        ; Aseguramos que el valor es de 32 bits
    mov     BYTE [rdx+16], al

    ; Escribir el puntero a data en offset +24
    mov     rax, QWORD [rbp-32]       ; Cargar el puntero a data
    mov     QWORD [rdx+24], rax

    ; Devolver el puntero al nodo en rdx
    mov     rax, rdx

    mov     rsp, rbp
    pop     rbp
    ret

string_proc_list_add_node_asm:
    ; Prólogo
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32                  ; Reservamos espacio en stack
    push    r12
    push    r13

    ; Guardar argumentos: list (rdi), type (rsi), data/hash (rdx)
    mov     QWORD [rbp-32], rdi      ; Guardar list
    mov     BYTE  [rbp-24], sil      ; Guardar type (1 byte)
    mov     QWORD [rbp-16], rdx      ; Guardar data/hash

    ; Preparar argumentos para string_proc_node_create_asm(type, data)
    movzx   edi, BYTE [rbp-24]       ; Cargar type como entero (32 bits)
    mov     rsi, QWORD [rbp-16]      ; Cargar data/hash
    call    string_proc_node_create_asm

    ; rax contiene el nuevo nodo creado
    mov     rdx, rax                 ; Guardamos nodo en rdx

    ; Recuperar list desde stack
    mov     r12, QWORD [rbp-32]

    ; Verificar si list->first == NULL
    cmp     QWORD [r12], 0
    jne     .not_empty               ; Si no es NULL, la lista no está vacía

    ; Lista vacía: inicializar first y last
    mov     [r12], rdx               ; list->first = node
    mov     [r12+8], rdx             ; list->last = node
    mov     rax, rdx                 ; Devolver el nodo insertado
    pop     r13
    pop     r12
    mov     rsp, rbp
    pop     rbp
    ret

.not_empty:
    ; Lista no vacía: insertar al final
    mov     r13, [r12+8]             ; r13 = list->last
    mov     [r13], rdx               ; last->next = node
    mov     [rdx+8], r13             ; node->prev = last
    mov     [r12+8], rdx             ; list->last = node
    mov     rax, rdx                 ; Devolver nodo insertado
    pop     r13
    pop     r12
    mov     rsp, rbp
    pop     rbp
    ret


string_proc_list_concat_asm:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32                  ; Reservar espacio para variables locales
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    ; Guardar argumentos
    mov     QWORD [rbp-24], rdi      ; list
    mov     BYTE  [rbp-16], sil      ; type
    mov     QWORD [rbp-8], rdx       ; hash

    ; Verificar si list es NULL
    test    rdi, rdi
    jnz     .alloc_str               ; Si no es NULL, seguimos
    xor     rax, rax                 ; Si es NULL, retornar NULL
    jmp     .cleanup

.alloc_str:
    ; Reservar 1 byte para string vacío (new_hash)
    mov     edi, 1
    call    malloc
    test    rax, rax
    jnz     .init_loop               ; Si malloc fue exitoso, continuar
    xor     rax, rax                 ; Si falló, retornar NULL
    jmp     .cleanup

.init_loop:
    mov     r14, rax                 ; r14 = new_hash
    mov     byte [r14], 0            ; Inicializar con string vacío ('\0')

    mov     rbx, [rbp-24]            ; rbx = list
    mov     r15, [rbx]               ; r15 = list->first
    movzx   r12, BYTE [rbp-16]       ; r12 = type (convertido a 32 bits)

.loop:
    ; Verificar si terminamos de recorrer la lista
    test    r15, r15
    jz      .after_loop

    ; Comparar type del nodo actual con el buscado
    mov     al, [r15+16]             ; al = current_node->type
    cmp     al, r12b
    je      .concat_node             ; Si coincide, concatenamos

.skip_node:
    ; Avanzar al siguiente nodo
    mov     r15, [r15]               ; current_node = current_node->next
    jmp     .loop

.concat_node:
    ; Concatenar new_hash con current_node->hash
    mov     rdi, r14
    mov     rsi, [r15+24]
    call    str_concat
    test    rax, rax
    jz      .fail                    ; Si str_concat falló, retornar NULL

    ; Liberar el antiguo new_hash y actualizar con el nuevo
    mov     rdi, r14
    mov     r14, rax
    call    free
    jmp     .skip_node

.after_loop:
    ; Si se pasó un hash original, concatenar también con new_hash
    mov     r13, [rbp-8]
    test    r13, r13
    jz      .success_return

    mov     rdi, r13
    mov     rsi, r14
    call    str_concat
    test    rax, rax
    jz      .fail

    ; Liberar el anterior new_hash y guardar el nuevo
    mov     rdi, r14
    mov     r14, rax
    call    free

.success_return:
    mov     rax, r14                 ; Retornar new_hash
    jmp     .cleanup

.fail:
    xor     rax, rax                 ; Retornar NULL

.cleanup:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    mov     rsp, rbp
    pop     rbp
    ret
