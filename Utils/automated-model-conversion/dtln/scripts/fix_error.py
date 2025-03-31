in_file = "convert_weights_to_tf_lite.py"

with open(in_file, "r+") as f:
    data=f.read()
    f.truncate(0)
    f.seek(0)
    f.write(data.replace("'False'", 'False'))