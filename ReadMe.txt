Sarah Phillips
IGME 590 Assignment 9

I chose to implement a compute shader that uses a cellular noise algorithm (found at https://thebookofshaders.com/12/) to output a waving texture. This texture is then multiplied with the object's normal map and surface color (specifically the green value). One material, applied to the first sphere, also uses the texture as its albedo.