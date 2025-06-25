
const gw    = location.origin;              // Gateway ESP32-WROOM
const proxy = "http://192.168.0.10:8000";   // PC (FastAPI + Google Vision)
const CAM   = "http://192.168.0.109:81/stream"; // flux MJPEG ESP32-CAM

/* elemente din pagină */
const pad        = document.getElementById("pad");
const sl         = document.getElementById("sl");
const v          = document.getElementById("v");
const btnDesc    = document.getElementById("btnDesc");
const captionBox = document.getElementById("caption");

/* porneşte fluxul video */
v.src = CAM;

/* coadă simplă de comenzi – evită flood */
let busy = false;
function send(cmd) {
  if (busy) return;
  busy = true;
  fetch(`${gw}/${cmd}`, { cache: "no-store" })
    .catch(() => {})          // ignoră timeout
    .finally(() => (busy = false));
  setTimeout(() => (busy = false), 800);
}

/* butoane  */
pad.onclick = (e) => {
  const c = e.target.dataset.c;
  if (c) send(c);
};

/* slider servo */
let debounce = 0;
sl.oninput = (e) => {
  clearTimeout(debounce);
  debounce = setTimeout(() => {
    send("P" + e.target.value + "\n");
  }, 140);
};


function setCaption(text) {
  if (captionBox) captionBox.textContent = text;
  else             alert(text);
}


async function describe() {
  setCaption("Analizez imaginea…");

  try {
    /* 1) JPEG de la Gateway */
    const blob = await fetch(`${gw}/capture`, { cache: "no-store" })
                         .then((r) => r.blob());

    /* 2) Trimite la Vision */
    const fd = new FormData();
    fd.append("file", blob, "snap.jpg");

    const data = await fetch(`${proxy}/vision`, {
                  method: "POST",
                  body: fd,
                }).then((r) => r.json());

    /* 3) Afişează rezultatul */
    if (data.description) {
      setCaption(data.description);
    } else {
      setCaption("Eroare Vision: " + (data.error || "răspuns invalid"));
    }
  } catch (err) {
    setCaption("Eroare conexiune: " + err);
  }
}

btnDesc.onclick = describe;
