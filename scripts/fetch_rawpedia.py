import argparse
import json
import os
import re
import requests
from bs4 import BeautifulSoup
from markdownify import markdownify as md

API_URL = "https://rawpedia.rawtherapee.com/api.php"

def get_all_pages(limit=None):
    """Fetch list of pages from the MediaWiki API."""
    pages = []
    params = {
        "action": "query",
        "list": "allpages",
        "apnamespace": 0, # Main namespace only
        "format": "json",
        "aplimit": "max" if not limit else limit
    }

    while True:
        response = requests.get(API_URL, params=params).json()
        if "query" in response and "allpages" in response["query"]:
            for page in response["query"]["allpages"]:
                pages.append(page["title"])

        if limit and len(pages) >= limit:
            pages = pages[:limit]
            break

        if "continue" in response:
            params.update(response["continue"])
        else:
            break

    return pages

def get_page_html(title):
    """Fetch the parsed HTML content of a page."""
    params = {
        "action": "parse",
        "page": title,
        "format": "json",
        "prop": "text"
    }
    response = requests.get(API_URL, params=params).json()
    if "parse" in response and "text" in response["parse"]:
        return response["parse"]["text"]["*"]
    return None

def clean_html(html):
    """Use BeautifulSoup to remove scripts, styles, and unwanted navigation."""
    soup = BeautifulSoup(html, "html.parser")

    # Remove unwanted elements
    for elem in soup.find_all(['script', 'style', 'nav', 'footer']):
        elem.decompose()

    # Remove standard MediaWiki UI elements that get parsed
    for elem in soup.find_all(class_=['mw-editsection', 'toc', 'navbox']):
        elem.decompose()

    return str(soup)

def convert_to_markdown(html):
    """Convert clean HTML to markdown using markdownify."""
    # Convert HTML to Markdown
    markdown = md(html, heading_style="ATX", autolinks=False)

    # Clean up excess newlines
    markdown = re.sub(r'\n{3,}', '\n\n', markdown)
    return markdown.strip()

def main():
    parser = argparse.ArgumentParser(description="Fetch and convert RawPedia to Markdown for RAG.")
    parser.add_argument("--outdir", "-o", default="data/rawpedia", help="Output directory")
    parser.add_argument("--sample", "-s", type=int, default=None, help="Download only a small sample of pages")
    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    print(f"Fetching page list from {API_URL}...")
    pages = get_all_pages(limit=args.sample)
    print(f"Found {len(pages)} pages to process.")

    for title in pages:
        print(f"Processing '{title}'...")
        html = get_page_html(title)
        if html:
            clean = clean_html(html)
            markdown = convert_to_markdown(clean)

            # Create a safe filename
            safe_title = re.sub(r'[^a-zA-Z0-9_\-]', '_', title)
            out_file = os.path.join(args.outdir, f"{safe_title}.md")

            with open(out_file, "w", encoding="utf-8") as f:
                f.write(f"# {title}\n\n{markdown}")
        else:
            print(f"Failed to fetch content for '{title}'")

    print(f"Done! Saved {len(pages)} articles to '{args.outdir}'.")

if __name__ == "__main__":
    main()
