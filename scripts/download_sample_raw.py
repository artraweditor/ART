import os
import urllib.request

def download_sample():
    url = "https://www.rawsamples.ch/raws/nikon/d70/RAW_NIKON_D70.NEF"
    out_dir = "data/fivek_sample"
    out_file = os.path.join(out_dir, "sample1.nef")

    os.makedirs(out_dir, exist_ok=True)

    print(f"Downloading sample RAW to {out_file}...")
    try:
        urllib.request.urlretrieve(url, out_file)
        print("Downloaded real RAW file.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    download_sample()
