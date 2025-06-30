import sys
import webbrowser
import pyperclip
import pyautogui
import threading
import re
import time
import requests
from youtube_transcript_api import YouTubeTranscriptApi
from youtube_transcript_api.formatters import TextFormatter

# API engelleme karşıtı ayarlar
def setup_session():
    session = requests.Session()
    # Farklı browser user agent'ları rotasyonu
    user_agents = [
        'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
        'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/121.0',
        'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.1 Safari/605.1.15',
        'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36'
    ]
    
    import random
    session.headers.update({
        'User-Agent': random.choice(user_agents),
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8',
        'Accept-Language': 'en-US,en;q=0.9,tr;q=0.8',
        'Accept-Encoding': 'gzip, deflate, br',
        'Connection': 'keep-alive',
        'Upgrade-Insecure-Requests': '1',
        'Sec-Fetch-Dest': 'document',
        'Sec-Fetch-Mode': 'navigate',
        'Sec-Fetch-Site': 'none',
        'Sec-Ch-Ua': '"Not_A Brand";v="8", "Chromium";v="120", "Google Chrome";v="120"',
        'Sec-Ch-Ua-Mobile': '?0',
        'Sec-Ch-Ua-Platform': '"Windows"',
        'DNT': '1',
        'Cache-Control': 'max-age=0'
    })
    return session

def extract_video_id(url):
    return (url.split("youtu.be/")[1].split("?")[0] if "youtu.be/" in url 
            else url.split("v=")[1].split("&")[0] if "youtube.com/watch" in url 
            else None)

def get_transcript_fast(video_id):
    # Farklı yöntemlerle deneme
    methods = [
        lambda: get_with_custom_session(video_id),
        lambda: get_with_fallback(video_id),
        lambda: get_with_basic_api(video_id)
    ]
    
    for i, method in enumerate(methods):
        try:
            print(f'Yöntem {i+1} deneniyor...')
            result = method()
            if result[0] is not None:
                return result
            time.sleep(1)  # Kısa bekleme
        except Exception as e:
            print(f'Yöntem {i+1} başarısız: {str(e)[:100]}...')
            time.sleep(2)
    
    return None, None

def get_with_custom_session(video_id):
    session = setup_session()
    
    try:
        # Yeni API sürümü için güncellenmiş çağrı
        transcript_list = YouTubeTranscriptApi.list_transcripts(video_id)
        
        available_langs = [t.language_code for t in transcript_list]
        print(f'Mevcut diller: {available_langs}')
        
        # Öncelik sırası: 1. Türkçe, 2. İngilizce, 3. Otomatik/diğer
        priority_languages = ['tr', 'en']
        
        for lang_code in priority_languages:
            for transcript_item in transcript_list:
                if transcript_item.language_code == lang_code:
                    try:
                        print(f'Dil bulundu: {lang_code}')
                        # Session ile özel çağrı
                        data = transcript_item.fetch()
                        return data, lang_code
                    except Exception as e:
                        print(f'Dil {lang_code} için hata: {str(e)[:50]}...')
                        continue

        # Diğer dilleri dene
        for transcript_item in transcript_list:
            try:
                print(f'Alternatif dil: {transcript_item.language_code}')
                data = transcript_item.fetch()
                return data, transcript_item.language_code 
            except Exception as e:
                print(f'Alternatif dil hatası: {str(e)[:50]}...')
                continue
                
    except Exception as e:
        print(f'Custom session hatası: {str(e)[:100]}...')
    
    return None, None

def get_with_fallback(video_id):
    try:
        # Direct API çağrısı (yeni sürüm)
        priority_languages = ['tr', 'en']
        
        for lang_code in priority_languages:
            try:
                transcript = YouTubeTranscriptApi.get_transcript(video_id, languages=[lang_code])
                print(f'Fallback başarılı: {lang_code}')
                return transcript, lang_code
            except Exception as e:
                print(f'Fallback {lang_code} hatası: {str(e)[:50]}...')
                continue
                
        # Son çare: İlk mevcut dili al
        try:
            transcript = YouTubeTranscriptApi.get_transcript(video_id)
            return transcript, 'auto'
        except Exception as e:
            print(f'Auto fallback hatası: {str(e)[:50]}...')
            
    except Exception as e:
        print(f'Fallback hatası: {str(e)[:100]}...')
        
    return None, None

def get_with_basic_api(video_id):
    try:
        # En basit API çağrısı (yeni sürümle)
        transcript = YouTubeTranscriptApi.get_transcript(video_id)
        print('Basic API başarılı')
        return transcript, 'basic'
    except Exception as e:
        print(f'Basic API hatası: {str(e)[:100]}...')
        
    return None, None

def fix_transcript_syntax(text):
    # Normalize whitespace: replace multiple newlines, tabs, spaces with a single space
    text = re.sub(r'[\n\t]+', ' ', text) 
    text = re.sub(r'\s{2,}', ' ', text).strip()

    # Remove spaces before common punctuation
    text = re.sub(r'\s+([.!?,:;])', r'\1', text)
    
    # Ensure a single space after common punctuation, unless it's the end of the string or followed by another punctuation
    text = re.sub(r'([.!?,:;])(?=[a-zA-Z0-9])', r'\1 ', text)

    # Remove multiple punctuation marks (e.g., "!!" -> "!", ",," -> ",")
    text = re.sub(r'([.!?]){2,}', r'\1', text)
    text = re.sub(r'([,:;]){2,}', r'\1', text)

    # Capitalize the first letter of the text
    if text and text[0].islower():
        text = text[0].upper() + text[1:]

    # Capitalize the first letter after sentence-ending punctuation (. ! ?)
    # This regex tries to handle cases where there might be multiple spaces or no space after punctuation.
    def capitalize_sentence(match):
        return match.group(1) + match.group(2).upper()
    text = re.sub(r'([.!?]\s*)([a-z])', capitalize_sentence, text)
    
    # Further clean up: ensure no leading/trailing spaces on lines if any newlines were reintroduced (unlikely with current logic but good practice)
    text = "\n".join([line.strip() for line in text.split('\n')])
    text = re.sub(r'\s{2,}', ' ', text).strip() # Final pass for multiple spaces

    return text

def open_gemini_and_paste():
    # Gemini sitesini aç
    webbrowser.open('https://gemini.google.com/')
    
    # Sitenin yüklenmesi için bekle
    time.sleep(3)
    
    # Chat alanını bulmak için birkaç deneme yap
    attempts = 0
    max_attempts = 10
    
    while attempts < max_attempts:
        try:
            # Gemini chat alanına odaklanmak için Tab tuşunu kullan
            if attempts == 0:
                # İlk denemede sayfanın ortasına tıkla
                pyautogui.click(pyautogui.size()[0] // 2, pyautogui.size()[1] // 2)
                time.sleep(0.5)
            
            # Chat input alanını bulmak için farklı yöntemler dene
            if attempts < 3:
                # Yöntem 1: Tab ile input alanına git
                for _ in range(attempts + 1):
                    pyautogui.press('tab')
                    time.sleep(0.1)
            elif attempts < 6:
                # Yöntem 2: Sayfanın alt kısmına tıkla
                pyautogui.click(pyautogui.size()[0] // 2, int(pyautogui.size()[1] * 0.8))
                time.sleep(0.5)
            else:
                # Yöntem 3: Ctrl+A ile tümünü seç ve yapıştır
                pyautogui.hotkey('ctrl', 'a')
                time.sleep(0.2)
            
            # Yapıştırma işlemini dene
            pyautogui.hotkey('ctrl', 'v')
            time.sleep(0.5)
            
            # Başarılı olduğunu varsay ve çık
            print('Gemini chat alanına yapıştırma başarılı!')
            return True
            
        except Exception as e:
            print(f'Yapıştırma denemesi {attempts + 1} başarısız: {str(e)[:50]}...')
            
        attempts += 1
        time.sleep(1)
    
    print('Gemini chat alanına yapıştırma başarısız oldu.')
    return False

if __name__ == "__main__":
    video_id = extract_video_id(sys.argv[1]) if len(sys.argv) == 2 else None
    if not video_id:
        print('Error: Invalid YouTube URL provided via command line.')
        sys.exit(1)
    
    print('Status: Getting transcript from youtube_transcript_api...')
    transcript, lang = get_transcript_fast(video_id)
    
    if transcript:
        # Yeni API sürümü için güncellenen veri işleme
        if hasattr(transcript[0], 'text'):
            # Yeni format: FetchedTranscriptSnippet objects
            raw_text = " ".join(entry.text for entry in transcript)
        else:
            # Eski format: dictionary
            raw_text = " ".join(entry['text'] for entry in transcript)
            
        fixed_text = fix_transcript_syntax(raw_text)
        
        # Copy to clipboard
        final_text = fixed_text + " lütfen bu transkriptin kapsamlı ve ayrıntılı bir özetini Türkçe olarak sağlayın"
        pyperclip.copy(final_text)
        
        print(f"Status: Transcript accessed (language: {lang})")
        print('Status: Opening Gemini and pasting...')
        
        # Gemini'yi aç ve yapıştır
        success = open_gemini_and_paste()
        
        if success:
            print('Status: Successfully pasted to Gemini chat!')
        else:
            print('Status: Transcript copied to clipboard. Please paste manually to Gemini.')
            
    else:
        print('Error: No transcript found or could not be fetched.')
        sys.exit(1)
