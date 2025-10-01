from flask import Flask, jsonify, send_from_directory, render_template_string, request, redirect, url_for
import os
from PIL import Image
import pillow_heif
from io import BytesIO
from threading import Lock
from werkzeug.middleware.proxy_fix import ProxyFix

app = Flask(__name__)
PHOTOS_DIR = os.path.join(os.path.dirname(__file__), 'photos')
latest_msg = ""
# _latest_lock = Lock()

@app.route('/')
def index():
    # Serve the HTML directly from file
    with open(os.path.join(os.path.dirname(__file__), './static/image.html')) as f:
        html = f.read()
    return render_template_string(html)

@app.route('/api/images')
def list_images():
    images = [f for f in os.listdir(PHOTOS_DIR) if f.lower().endswith(('.jpg', '.jpeg', '.png', '.gif'))]
    return jsonify(images)

@app.route('/photos/<filename>')
def serve_image(filename):

    return send_from_directory(PHOTOS_DIR, filename)


@app.route('/api/convertPhotos')
def convertPhotos():
    images = [f for f in os.listdir(PHOTOS_DIR) if (f.lower().endswith(('.jpg', '.jpeg', '.png', '.gif', '.heic')) and not f.startswith('BL_'))]
    for img in images:
        imgPath = os.path.join(PHOTOS_DIR, img)
        # Always save as .jpeg, even for HEIC
        newFile = 'BL_' + os.path.splitext(img)[0] + '.jpeg'
        newPath = os.path.join(PHOTOS_DIR, newFile)

        # CHeck if the new file already exists
        if os.path.exists(newPath):
            os.remove(imgPath) # remove dupe
            continue 

        targetSize = (320, 480)

        if img.lower().endswith('.heic'):
            heif_file = pillow_heif.read_heif(imgPath)
            im = Image.frombytes(
                heif_file.mode,
                heif_file.size,
                heif_file.data,
                "raw"
            )
        else:
            im = Image.open(imgPath)

        im = im.convert('RGB')
        im.thumbnail(targetSize, Image.LANCZOS)
        im.save(newPath, "JPEG", quality=60, progressive=False, optimize=True)
        im.close()
        os.remove(imgPath)
    return jsonify({"status": "All images converted"})

@app.route('/upload', methods=['POST'])
def upload_image():
    files = request.files.getlist('file')
    saved_files = []
    for file in files:
        if file.filename == '':
            continue
        save_path = os.path.join(PHOTOS_DIR, file.filename)
        file.save(save_path)
        saved_files.append(file.filename)
    return redirect(url_for('index'))


@app.route('/api/deleteImage', methods=['POST'])
def receive_imgsrc():
    data = request.get_json()
    img_src = data.get('imgSrc')
    img = img_src.split('/')
    os.remove(os.path.join(PHOTOS_DIR, img[4]))
    return redirect(url_for('index'))

@app.route('/api/sendMessage', methods=['GET', 'POST'])
def send_message():
    global latest_msg
    if request.method == 'POST':
        data = request.get_json(silent=True) or {}
        msg = str(data.get('message', ''))
        latest_msg = msg
        return {'ok': True}, 200

    txt = latest_msg

    if request.method == 'GET':
        latest_msg = ''
    
    return (txt, 200, {
        'Content-Type': 'text/plain; charset=utf-8',
        'Cache-Control': 'no-store'  
    })

if __name__ == '__main__':
    app.wsgi_app = ProxyFix(
    app.wsgi_app, x_for=1, x_proto=1, x_host=1, x_prefix=1)
    app.run(debug=True, host='0.0.0.0', port=5000)
