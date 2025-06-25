from fastapi import FastAPI, UploadFile, File
from fastapi.middleware.cors import CORSMiddleware
from google.cloud import vision
from dotenv import load_dotenv

load_dotenv()

client = vision.ImageAnnotatorClient()

app = FastAPI(title="Cam-Bot Vision Proxy")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["POST"],
    allow_headers=["*"],
)

@app.post("/vision")
async def vision_endpoint(file: UploadFile = File(...)):
    img_bytes = await file.read()
    image = vision.Image(content=img_bytes)

    response = client.label_detection(image=image)
    if response.error.message:
        return {"error": response.error.message}

    labels = [l.description for l in response.label_annotations]
    return {"description": ", ".join(labels) or "Nimic detectat"}
