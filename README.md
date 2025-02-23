 
# Optical Flow Visualization with Lucas-Kanade

This repository contains a C implementation of the Lucas-Kanade optical flow algorithm to compute and visualize motion between consecutive frames of a video or image sequence. The code processes JPG frames, calculates the optical flow field, and overlays flow vectors as green arrows on the original images. The output can be stitched back into a video using FFmpeg.

## Features
- **Grayscale Conversion**: Converts RGB JPG images to grayscale for flow computation.
- **Lucas-Kanade Algorithm**: Computes optical flow between consecutive frames using spatial gradients and window-based matching.
- **Visualization**: Draws green flow vectors on the original RGB images using Bresenham's line algorithm.
- **Batch Processing**: Processes a sequence of frames from an input folder and saves results to an output folder.
- **Dependencies**: Uses the lightweight `stb_image.h` and `stb_image_write.h` libraries for image I/O.

## Prerequisites
- **C Compiler**: GCC or any compatible compiler (e.g., `gcc` on Linux/Mac, MinGW on Windows).
- **FFmpeg**: Required for splitting videos into frames and stitching output frames into a video.
- **Input Frames**: A sequence of JPG images (e.g., `frame_0001.jpg`, `frame_0002.jpg`, etc.) in a folder named `frames_input`.

## Installation
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/lazycodebaker/Lucas-Kanade
   cd optical-flow-lucas-kanade
   ```

2. **Download STB Libraries**:
   - The code uses `stb_image.h` and `stb_image_write.h`. These are single-header libraries included via `#define` directives.
   - If not already present, download them from [stb GitHub](https://github.com/nothings/stb) and place them in the project directory.

3. **Install FFmpeg**:
   - On Ubuntu: `sudo apt install ffmpeg`
   - On macOS: `brew install ffmpeg`
   - On Windows: Download from [FFmpeg official site](https://ffmpeg.org/download.html) and add to PATH.

## Compilation
Compile the code using a C compiler:
```bash
gcc -o optical_flow main.c -lm
```
The `-lm` flag links the math library for functions like `fabs`.

## Usage
1. **Prepare Input Frames**:
   - Convert a video to frames using FFmpeg:
     ```bash
     mkdir frames_input
     ffmpeg -i input.mp4 frames_input/frame_%04d.jpg
     ```
   - Ensure frames are named sequentially (e.g., `frame_0001.jpg`, `frame_0002.jpg`).

2. **Run the Program**:
   - Create an output directory:
     ```bash
     mkdir frames_output
     ```
   - Execute the compiled binary:
     ```bash
     ./optical_flow
     ```
   - The program processes up to 100 frames (configurable via `MAX_FRAMES`) and saves flow visualizations to `frames_output/flow_XXXX.jpg`.

3. **Generate Output Video**:
   - Convert the output frames back to a video:
     ```bash
     ffmpeg -framerate 30 -i frames_output/flow_%04d.jpg -c:v libx264 -pix_fmt yuv420p output.mp4
     ```

## Example
- Input: A video `input.mp4` with moving objects.
- Output: `output.mp4` showing green arrows indicating motion between frames.

## Code Structure
- **Image Loading**: `readJPG` converts JPG to grayscale; `loadRGB` loads RGB for visualization.
- **Gradient Computation**: `computeGradients` calculates spatial gradients (Ix, Iy).
- **Flow Calculation**: `lucasKanade` implements the Lucas-Kanade method with a sliding window.
- **Visualization**: `saveFlowImage` draws flow vectors using `drawLine`.
- **Main Loop**: Processes frames sequentially, computing flow between pairs.

## Configuration
- **Window Size**: Adjust `lucasKanade` call in `main` (currently `5`) for larger/smaller flow windows.
- **Frame Limit**: Modify `MAX_FRAMES` in `main` to process more/fewer frames.
- **Arrow Density**: Change the step size (currently `10`) in `saveFlowImage` to adjust arrow frequency.

## Limitations
- Assumes consistent frame sizes across the sequence.
- No multi-threading; processes frames sequentially.
- Flow vectors are visualized every 10 pixels for clarity—may miss fine details.

## Dependencies
- `stb_image.h`: Image loading (JPG).
- `stb_image_write.h`: Image saving (JPG).
- Standard C libraries: `stdio.h`, `stdlib.h`, `math.h`, `string.h`.

## License
This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
