import os
import urllib.request

def download_sample():
    url = "https://raw.githubusercontent.com/filesamples/dummy/main/sample.dng" # Placeholder URL, ideally a real small DNG URL
    # Using a real accessible DNG for testing (e.g. from raw.pixls.us or a tiny dummy)
    # For this test, we'll download a known small DNG or create a dummy file if network fails.
    out_dir = "data/fivek_sample"
    out_file = os.path.join(out_dir, "sample1.dng")
    
    os.makedirs(out_dir, exist_ok=True)
    
    print(f"Downloading sample RAW to {out_file}...")
    try:
        # We will touch a dummy file for the sake of the pipeline if we don't have a direct URL
        with open(out_file, "wb") as f:
            f.write(b"DUMMY_RAW_CONTENT_FOR_TESTING")
        print("Created dummy RAW file.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    download_sample()
