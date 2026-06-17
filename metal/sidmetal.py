import os, subprocess 
import ctypes


"""

 xcrun -sdk macosx metal -c add.metal -o add.air
 xcrun -sdk macosx metallib add.air -o add.metallib
 clang main.m \\n-framework Metal \\n-framework Foundation \\n-o add_program
 ./add_program


"""


PATH = os.getcwd()
print(PATH)

proc = subprocess.Popen(
    ["python", "--version"],
    stdout=subprocess.PIPE,
    text=True
)
out, _ = proc.communicate()
print(out)



lib_path = os.path.abspath(".")