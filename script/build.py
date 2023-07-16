import json
import sys
import os

class build_project:
    def __init__(self, path):
        with open(path, "r", encoding='utf-8') as f:
            json_data = json.loads(f.read())
        self.project_name = json_data["project_name"]
        self.project_version = json_data["project_version"]
        self.cpp_version = json_data["cpp_version"]
        self.third_party = json_data["third_party"]
        self.library_out = json_data["library_out"]
    def build_third_party(self):
        
        cwd = os.getcwd()
        for name, library in self.third_party.items():
            if library["enable"] == True:
                interprter = library["interpreter"]
                build_script = library["build_script"]
                src_path = cwd + "/" + library["src_path"]
                out_path = cwd + "/" + library["out_path"]
                install_path = cwd + "/" + library["install_path"]

                build_cmd = "%s %s/%s build" % (interprter, out_path, build_script)
                clean_cmd = "%s %s/%s clean" % (interprter, out_path, build_script)
                install_cmd = "%s %s/%s install %s" % (interprter, out_path, build_script, install_path)
                print("======== building %s =========" % name)
                os.system("mkdir -p " + out_path)
                print("======== setup build env ======")
                os.system("cp -rf %s/* %s" % (src_path, out_path))
                os.chdir(out_path)
                print(os.getcwd())
                try:
                    print("clean command: " + clean_cmd)
                    os.system(clean_cmd)
                    print ("build command: " + build_cmd)
                    os.system(build_cmd)
                    print ("install command: " + install_cmd)
                    os.system(install_cmd)
                except:
                    print ("build %s failed!" % name)
                    sys.exit(1)
                finally:
                    os.chdir(cwd)
                print("======== building %s done =========" % name)
            else:
                print ("skip build: " + name)
    def clean_third_party(self):
        cwd = os.getcwd()
        for name, library in self.third_party.items():
            out_path = cwd + "/" + library["out_path"]
            os.system("rm -rf %s" % out_path)
    def clean_src_obj(self):
        os.system("rm -rf build/")
        os.system("rm -rf CMakeFiles/")
        os.system("rm -rf out/")
    def prepare(self):
        os.system("mkdir -p out/")
        os.system("mkdir -p out/lib")
        os.system("mkdir -p out/bin")
        os.system("mkdir -p build/")
    def build_src(self):
        cwd = os.getcwd()
        os.chdir("build/")
        os.system("cmake ..")
        os.system("make -j8")
        os.chdir(cwd)
def main(argv):
    if len(argv) != 2:
        sys.exit(1)
    build = build_project(argv[1])
    if argv[0] == "build":
        build.prepare()
        build.build_third_party()
        build.build_src()
    else:
        build.clean_third_party()
        build.clean_src_obj()

if __name__=="__main__":
    main(sys.argv[1:])