import os
import sys

def check_files(directory):
    #directory = os.path.abspath(directory)
    print(f"Checking folder: {directory}")  # Debugging print

    cc_files = []
    h_files = []
    
    for root, dirs, files in os.walk(directory, followlinks=True):  # Ensure it follows symlinks
        cc_files.extend([f for f in files if f.endswith(".cc")])
        h_files.extend([f for f in files if f.endswith(".h")])
    
    if not cc_files or not h_files:
        print(" ######################################################################## ")
        print(" ######################################################################## ")
        print(" ERROR: Missing required .cc or .h model files in common\model folder!. Please refer the README.md in the application to generate them. ")  # Red error message
        print(" ######################################################################## ")
        print(" ######################################################################## ")
        sys.exit(1)
    else:
        print(" ######################################################################## ")
        print(" All required .cc and .h model files are present! ")  # Green success message
        print(" ######################################################################## ")

if __name__ == "__main__":
    folder_path = sys.argv[1] if len(sys.argv) > 1 else os.getcwd()
    check_files(folder_path)
