def to_word(x):
    s = ""
    while True:
        s += chr(ord('a') + x % 26)
        x //= 26
        if x == 0:
            break
    return s

n = 50000
with open("test.txt", "w") as f:
    for i in range(n):
        f.write(f"+ {to_word(i)} {i}\n")
    for i in range(n):
        f.write(f"{to_word(i)}\n")
    for i in range(n):
        f.write(f"- {to_word(i)}\n")
    for i in range(n):
        f.write("! Save dump.bin\n")