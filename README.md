# YouTube Link Copier

This C++ application automatically copies YouTube links selected in your browser and provides additional functionality for working with YouTube video transcripts.

## Features

- **Global Hotkey**: Select a link in any browser and press `Ctrl+Y`
- **YouTube Link Recognition**: Only captures YouTube links
- **Automatic Copying**: Copies the link to clipboard
- **Modern GUI Panel**: Shows a modern notification panel in the bottom right corner
- **Auto-close**: Automatically disappears after 5 seconds
- **Close Button**: Can be closed manually
- **Transcript Helper**: Includes a Python script to extract and process YouTube video transcripts

## Installation

### Pre-compiled Binary (Windows)
1. Download the latest `YouTubeLinkCopier.exe` from the [Releases](https://github.com/yourusername/youtube-link-copier/releases) page
2. Run the executable directly - no installation required

### Build from Source
1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/youtube-link-copier.git
   cd youtube-link-copier
   ```

2. Build using MinGW:
   ```bash
   mingw32-make
   ```

3. Or build using CMake:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

## Requirements

### For the C++ Application
- Windows 10/11
- MinGW-w64 (G++ compiler) if building from source
- C++17 support

### For the Transcript Helper (get_transcript.py)
- Python 3.6+
- Required Python packages:
  ```bash
  pip install youtube-transcript-api pyperclip pyautogui
  ```

## Usage

### Link Copier
1. Run the `YouTubeLinkCopier.exe` file
2. Console window will show "YouTube Link Copier started!"
3. Select a YouTube link in any browser (highlight it)
4. Press `Ctrl+Y`
5. The link will be automatically copied and a notification panel will appear

### Transcript Helper
The included Python script helps you extract transcripts from YouTube videos:

1. Install the required Python packages as mentioned above
2. Run the script with a YouTube URL:
   ```bash
   python get_transcript.py "https://www.youtube.com/watch?v=VIDEO_ID"
   ```
3. The script will:
   - Extract the video transcript
   - Format it properly
   - Open Google Gemini
   - Paste the transcript with a request for summarization

## Supported YouTube Link Formats

- `https://youtube.com/watch?v=...`
- `https://www.youtube.com/watch?v=...`
- `https://youtu.be/...`
- `https://youtube.com/embed/...`
- `https://youtube.com/v/...`

## Technical Details

- Global hotkey capture using Windows API
- Clipboard operations
- YouTube link detection with regex
- Modern GUI panel notification with flat design
- Transcript extraction and processing via YouTube API
- Automatic browser interaction for AI processing

## Project Structure

- `main.cpp` - Main C++ application source
- `get_transcript.py` - Python script for transcript extraction
- `CMakeLists.txt` - CMake configuration
- `Makefile` - Make configuration for direct building

## Troubleshooting

- **Link not copying**: Ensure the link is properly selected and is a valid YouTube URL
- **Hotkey not working**: Make sure no other application is using the `Ctrl+Y` hotkey
- **Transcript extraction failing**: Check your internet connection and verify the video has available captions
- **Build errors**: Ensure you have the correct compiler version and all required dependencies

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Feel free to submit a Pull Request. 