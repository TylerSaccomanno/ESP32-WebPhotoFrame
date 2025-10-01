    const index = []
    let curImg = 0
    
    document.addEventListener('DOMContentLoaded', function() {
        fetch('/api/convertPhotos')
        .then(response => response.json())

        
        fetch('/api/images')
            .then(response => response.json())
            .then(photos => {
            const container = document.getElementById('imageContainer');
            container.innerHTML = '';
                photos.forEach(photo => {
                    const indexedImg = {
                        imgSrc: '/photos/' + photo,
                    };

                    index.push(indexedImg);
                });

                updateImages();
                console.log(index);
            });

    });
    

function displayOverlay(imgSrc) {
    const overlay = document.getElementById('overlay');
    overlay.style.display = 'block';
    const uploadTxt = document.getElementById('uploadimg');
    const uploadTxt2 = document.getElementById('uploadTxt');
    const msgBoard = document.getElementById('msgBoard');
    const msgBTN = document.getElementById('sendMsg');
    const boardTxt = document.getElementById('boardTxt');

    uploadTxt.style.display = 'none';
    uploadTxt2.style.display = 'none';
    msgBoard.style.display = 'none';
    msgBTN.style.display = 'none';
    boardTxt.style.display = 'none';

    const deleteBTN = document.getElementById('confirmDelete');
    const cancelBTN = document.getElementById('cancelDelete');
    deleteBTN.onclick = function() {
        console.log('Pressed uyes');
        deleteImage(imgSrc);
    };

    cancelBTN.onclick = function() {
        overlay.style.display = 'none';
        uploadTxt.style.display = 'block';
        uploadTxt2.style.display = 'block';
        uploadTxt2.style.marginLeft = '57.5%';
        uploadTxt2.style.marginTop = '-22.5px';
        msgBoard.style.display = 'block';
        msgBTN.style.display = 'block';
        boardTxt.style.display = 'block';

    };

}

function updateImages() {
    const container = document.getElementById('imageContainer');
    container.innerHTML = '';
    for (let i = 0; i < index.length; i++) {
        const imgIndex = (curImg + i) % index.length;
        console.log(imgIndex);
        const imgBTN = document.createElement('button');
        imgBTN.style.position = 'relative';
        imgBTN.style.width = '100px';
        imgBTN.style.height = '100px';
        imgBTN.style.padding = '0';

        // add img to button
        const img = document.createElement('img');
        img.src = index[imgIndex].imgSrc;
        img.style.position = 'absolute';
        img.style.top = '0';
        img.style.left = '0';
        img.style.width = '100%';
        img.style.height = '100%';


        imgBTN.onclick = function() {
            displayOverlay(img.src)
        };

        imgBTN.appendChild(img);
        container.appendChild(imgBTN);
    }
}

function deleteImage(imgSrc) {
    fetch('/api/deleteImage', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ imgSrc: imgSrc })
    })
    location.reload();
}

function sendMsg(){
  const el = document.getElementById('msgBoard');
  const val = el.value;                // read first
  el.value = '';                       // then clear
  fetch('/api/sendMessage', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({ message: val })
  })
  .then(r => r.json())
  .then(data => console.log('Server echoed:', data));
}