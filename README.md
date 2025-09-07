# SentryBOT
This open-source project provides a modular bipedal companion robot and a framework enhanced with specialized plugins. Designed to be customizable and extensible to user needs, this flexible framework is API-based and designed for use in robotics and automation projects. The project includes both hardware and software components.

## Camera stream submodule

We include the async FastAPI camera stream repo as a git submodule under `modules/camera-stream`, but only use the 4 root Python files. To clone with submodules:

- Fresh clone: `git clone --recurse-submodules <this-repo-url>`
- If already cloned: `git submodule update --init --recursive`

Update submodule to latest main:

```
git submodule update --remote --merge -- modules/camera-stream
git commit -m "Update camera-stream submodule"
```

Inside the submodule, only these files are relevant:

- `xStream_HW.py`
- `xStream_Software.py`
- `xStream_Webcam_Software.py`
- `xTestFps.py`

License and other files from the submodule are not used in this host repo.
