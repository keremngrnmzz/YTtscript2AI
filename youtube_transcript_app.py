import tkinter as tk
from tkinter import ttk
from youtube_transcript_api import YouTubeTranscriptApi
import pyperclip
import threading
import time
import keyboard
import webbrowser
import re
import urllib.parse
import pystray
from PIL import Image, ImageDraw
import sys

class YouTubeTranscriptManager:
    """YouTube transcript management class"""
    
    @staticmethod
    def extract_video_id(url):
        """Extract video ID from YouTube URL"""
        try:
            if "youtu.be" in url:
                return url.split("/")[-1].split("?")[0]
            elif "v=" in url:
                return urllib.parse.parse_qs(urllib.parse.urlparse(url).query)["v"][0]
            elif "embed" in url:
                return url.split("/")[-1].split("?")[0]
            elif re.match(r'^[A-Za-z0-9_-]{11}$', url):
                return url
        except:
            pass
        return None
    
    @staticmethod
    def get_transcript(video_id):
        """Get transcript by video ID"""
        try:
            transcript_list = YouTubeTranscriptApi.list_transcripts(video_id)
            for transcript in transcript_list:
                try:
                    return transcript.fetch()
                except:
                    continue
        except:
            pass
        return None
    
    @staticmethod
    def format_transcript(transcript_data):
        """Format transcript data into readable text"""
        if isinstance(transcript_data, list):
            return " ".join(item.get("text", "") for item in transcript_data)
        return str(transcript_data) if transcript_data else "Error"


class GUI(tk.Tk):
    """Minimal GUI - CTRL+Y trigger"""
    
    def __init__(self):
        super().__init__()
        
        self.title("YT")
        self.geometry("280x100")
        self.resizable(False, False)
        
        # Center window
        self.center_window()
        
        # System tray setup
        self.create_tray_icon()
        
        # Create widgets
        self.create_widgets()
        
        # Setup hotkey
        self.setup_hotkey()
        
        # Hide on start
        self.withdraw()
        
        # Bind protocol
        self.protocol("WM_DELETE_WINDOW", self.hide_window)
        
    def center_window(self):
        """Center window on screen"""
        self.update_idletasks()
        x = (self.winfo_screenwidth() - 280) // 2
        y = (self.winfo_screenheight() - 100) // 2
        self.geometry(f"280x100+{x}+{y}")
    
    def create_widgets(self):
        """Create minimal GUI"""
        frame = ttk.Frame(self, padding="8")
        frame.pack(fill="both", expand=True)
        
        ttk.Label(frame, text="YT", font=("Arial", 11, "bold")).pack()
        ttk.Label(frame, text="CTRL+Y", font=("Arial", 8)).pack(pady=2)
        
        self.status_var = tk.StringVar(value="Ready")
        ttk.Label(frame, textvariable=self.status_var, font=("Arial", 8)).pack()
        
        btn_frame = ttk.Frame(frame)
        btn_frame.pack(pady=4)
        
        ttk.Button(btn_frame, text="Hide", command=self.hide_window, width=6).pack(side="left", padx=2)
        ttk.Button(btn_frame, text="Exit", command=self.quit_app, width=6).pack(side="left", padx=2)
    
    def create_tray_icon(self):
        # Minimal icon
        image = Image.new('RGB', (16, 16), color='blue')
        draw = ImageDraw.Draw(image)
        draw.rectangle([4, 4, 12, 12], fill='white')
        
        menu = pystray.Menu(
            pystray.MenuItem("Show", self.show_window),
            pystray.MenuItem("Exit", self.quit_app)
        )
        
        self.icon = pystray.Icon("YT", image, "YT Transcript", menu)
        threading.Thread(target=self.icon.run, daemon=True).start()
    
    def show_window(self, icon=None, item=None):
        self.deiconify()
        self.lift()
    
    def hide_window(self):
        self.withdraw()
    
    def setup_hotkey(self):
        """Setup CTRL+Y hotkey"""
        try:
            keyboard.add_hotkey('ctrl+y', self.on_hotkey, suppress=True)
        except:
            self.status_var.set("Error")
    
    def on_hotkey(self):
        """Called when CTRL+Y is pressed"""
        try:
            keyboard.send('ctrl+c')
            time.sleep(0.02)
            text = pyperclip.paste().strip()
            self.after(0, lambda: self.process_text(text))
        except:
            self.after(0, lambda: self.status_var.set("Error"))
    
    def process_text(self, text):
        """Process copied text"""
        if not text or not ("youtube.com" in text or "youtu.be" in text):
            self.status_var.set("Invalid")
            self.after(1500, lambda: self.status_var.set("Ready"))
            return
        
        threading.Thread(target=self.process_url, args=(text,), daemon=True).start()
    
    def process_url(self, url):
        """Process YouTube URL"""
        try:
            self.status_var.set("Processing...")
            
            video_id = YouTubeTranscriptManager.extract_video_id(url)
            if not video_id:
                self.status_var.set("Error")
                self.after(1500, lambda: self.status_var.set("Ready"))
                return
            
            transcript_data = YouTubeTranscriptManager.get_transcript(video_id)
            if not transcript_data:
                self.status_var.set("No transcript")
                self.after(1500, lambda: self.status_var.set("Ready"))
                return
            
            transcript = YouTubeTranscriptManager.format_transcript(transcript_data)
            full_text = f"{transcript}\n\nPlease provide a comprehensive summary of this transcript in English"
            
            pyperclip.copy(full_text)
            webbrowser.open("https://gemini.google.com/")
            
            self.status_var.set("Opening...")
            time.sleep(1.8)
            keyboard.send('ctrl+v')
            
            self.status_var.set("Done")
            self.after(1500, lambda: self.status_var.set("Ready"))
            
        except:
            self.status_var.set("Error")
            self.after(1500, lambda: self.status_var.set("Ready"))
    
    def quit_app(self, icon=None, item=None):
        """Called when window is closed"""
        try:
            keyboard.unhook_all()  # Remove all keyboard hooks
            self.icon.stop()
        except:
            pass
        self.destroy()
        sys.exit()


def main():
    """Main function"""
    app = GUI()
    app.mainloop()


if __name__ == "__main__":
    main() 