import streamlit as st
import replicate
import os
import requests
from gtts import gTTS

# Configure the look of the app
st.set_page_config(page_title="AI Commercial Maker", page_icon="🎬", layout="centered")

# Securely grab the API key from Streamlit Secrets (for when it's deployed)
try:
    os.environ["REPLICATE_API_TOKEN"] = st.secrets["REPLICATE_API_TOKEN"]
except Exception:
    st.error("Please add your REPLICATE_API_TOKEN to the Streamlit Secrets!")

st.title("🎬 Smart AI Commercial Generator")
st.write("Type a vague idea, and the 'Brain' will automatically write the script, direct the visuals, and render the final video!")

tab1, tab2 = st.tabs(["🚀 Smart Promo Generator", "🖼️ Image to Video"])

# --- TAB 1: SMART PROMO GENERATOR (WITH THE BRAIN) ---
with tab1:
    st.subheader("What are you advertising?")
    user_idea = st.text_area("Example: I want a promo for a legal advice website called Rights Radar", "")
    
    if st.button("Generate Smart Commercial", use_container_width=True):
        if not user_idea:
            st.warning("Please enter an idea first!")
        else:
            with st.status("🧠 Engaging the Brain (Llama-3)...", expanded=True) as status:
                try:
                    # 1. THE BRAIN: Write the script and direct the visuals
                    st.write("✍️ Writing the script and directing the scene...")
                    
                    brain_output = replicate.run(
                        "meta/meta-llama-3-70b-instruct",
                        input={
                            "prompt": f"I want an ad for: {user_idea}",
                            "system_prompt": "You are an expert commercial director. Write a highly detailed, 1-sentence visual description for an AI video generator (focus on cinematic lighting, subject, and camera angles, no text on screen). Then write a short 1-sentence voiceover script. Format exactly like this:\nVISUAL: <visual description>\nSCRIPT: <voiceover script>",
                            "max_tokens": 200
                        }
                    )
                    brain_text = "".join(brain_output)
                    
                    # Parse the Brain's output
                    visual_prompt = "A cinematic shot of a professional office setting, 4k"
                    voice_script = "Welcome to our business, we are here to help."
                    for line in brain_text.split('\n'):
                        if line.startswith("VISUAL:"):
                            visual_prompt = line.replace("VISUAL:", "").strip()
                        elif line.startswith("SCRIPT:"):
                            voice_script = line.replace("SCRIPT:", "").strip()
                            
                    st.info(f"**🎥 Director's Vision (Visual Prompt):** {visual_prompt}")
                    st.info(f"**🎙️ Voiceover Script:** {voice_script}")
                    
                    # 2. THE VOICE: Generate Audio
                    st.write("🎙️ Recording AI Voiceover...")
                    tts = gTTS(text=voice_script, lang='en', tld='com')
                    tts.save("smart_voice.mp3")
                    st.audio("smart_voice.mp3")
                    
                    # 3. THE MUSCLE: Generate Video
                    st.write("🎥 Rendering cinematic video footage (Takes 1-3 minutes)...")
                    video_output = replicate.run(
                        "minimax/video-01",
                        input={"prompt": visual_prompt}
                    )
                    
                    vid_obj = video_output if not isinstance(video_output, list) else video_output[0]
                    
                    # Handle the FileOutput object
                    try:
                        vid_data = vid_obj.read()
                        st.video(vid_data)
                    except AttributeError:
                        st.video(str(vid_obj))
                        
                    status.update(label="✅ Commercial Ready!", state="complete", expanded=False)
                    st.success("Boom! Your smart commercial is ready to play above!")
                    
                except Exception as e:
                    status.update(label="❌ Error occurred", state="error")
                    st.error(f"Something went wrong: {e}")

# --- TAB 2: IMAGE TO VIDEO ---
with tab2:
    st.subheader("Animate an Uploaded Picture")
    uploaded_image = st.file_uploader("Upload a photo (JPG/PNG)", type=["jpg", "jpeg", "png"])
    
    if st.button("Animate this Picture", use_container_width=True):
        if uploaded_image is None:
            st.warning("Please upload a picture first!")
        else:
            temp_image_path = "temp_uploaded_image.jpg"
            with open(temp_image_path, "wb") as f:
                f.write(uploaded_image.getbuffer())
                
            with st.spinner("🎨 AI is animating your picture... (This takes 1-3 minutes)"):
                try:
                    output = replicate.run(
                        "stability-ai/stable-video-diffusion:3f0457e4619daac51203ddb4728167362e4f8c78c148386377e8a93a1523aef5",
                        input={
                            "cond_image": open(temp_image_path, "rb"),
                            "sizing_strategy": "maintain_aspect_ratio"
                        }
                    )
                    
                    video_output = output if not isinstance(output, list) else output[0]
                    
                    try:
                        video_data = video_output.read()
                        st.video(video_data)
                    except AttributeError:
                        st.video(str(video_output))
                    
                    st.success("✅ Image animated successfully!")
                    
                except Exception as e:
                    st.error(f"Something went wrong: {e}")
