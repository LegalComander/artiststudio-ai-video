import io
import os
import zipfile

import librosa
import numpy as np
import replicate
import requests
import streamlit as st
from gtts import gTTS

st.set_page_config(
    page_title="ArtistStudio",
    page_icon="🎛️",
    layout="wide",
    initial_sidebar_state="collapsed",
)

st.markdown(
    """
    <style>
    :root {
        --bg: #050812;
        --panel: rgba(7, 18, 36, 0.82);
        --line: rgba(0, 229, 255, 0.26);
        --cyan: #00e5ff;
        --blue: #3d7cff;
        --text: #eef8ff;
        --muted: #8aa4ba;
    }

    .stApp {
        background:
            radial-gradient(circle at 12% 6%, rgba(0, 166, 255, 0.18), transparent 28%),
            radial-gradient(circle at 90% 18%, rgba(0, 229, 255, 0.10), transparent 25%),
            linear-gradient(180deg, #03050b 0%, #06101c 52%, #03060d 100%);
        color: var(--text);
    }

    header[data-testid="stHeader"] { background: transparent; }
    #MainMenu, footer { visibility: hidden; }

    .block-container {
        max-width: 1280px;
        padding-top: 1.4rem;
        padding-bottom: 3rem;
    }

    .artist-hero {
        border: 1px solid rgba(0, 229, 255, 0.26);
        background: linear-gradient(135deg, rgba(3, 10, 22, .94), rgba(5, 31, 61, .77));
        border-radius: 26px;
        padding: 28px 32px;
        box-shadow: 0 0 60px rgba(0, 142, 255, 0.13), inset 0 0 35px rgba(0, 229, 255, 0.025);
        margin-bottom: 18px;
        position: relative;
        overflow: hidden;
    }

    .artist-hero:after {
        content: "";
        position: absolute;
        width: 280px;
        height: 280px;
        right: -100px;
        top: -150px;
        border-radius: 50%;
        background: rgba(0, 229, 255, 0.11);
        filter: blur(24px);
    }

    .brand {
        font-weight: 900;
        font-size: clamp(2rem, 5vw, 4.15rem);
        line-height: 1;
        letter-spacing: -0.04em;
        margin: 0;
        background: linear-gradient(90deg, #ffffff 0%, #9befff 42%, #3d7cff 100%);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        text-shadow: 0 0 34px rgba(0, 229, 255, 0.2);
    }

    .brand-sub {
        color: #9fb8cc;
        margin-top: 12px;
        font-size: 1.02rem;
        max-width: 760px;
    }

    .neon-pill {
        display: inline-block;
        padding: 6px 11px;
        margin-bottom: 13px;
        border-radius: 999px;
        color: #8ff5ff;
        border: 1px solid rgba(0, 229, 255, 0.30);
        background: rgba(0, 229, 255, 0.08);
        font-size: 0.78rem;
        font-weight: 800;
        letter-spacing: .12em;
        text-transform: uppercase;
    }

    .feature-card {
        border: 1px solid rgba(0, 229, 255, 0.20);
        background: linear-gradient(180deg, rgba(7, 18, 36, 0.90), rgba(4, 11, 22, 0.94));
        border-radius: 20px;
        padding: 20px;
        min-height: 144px;
        box-shadow: inset 0 0 28px rgba(0, 229, 255, 0.025);
    }

    .feature-title {
        font-size: 0.86rem;
        color: #7fdfff;
        text-transform: uppercase;
        letter-spacing: .1em;
        font-weight: 800;
        margin-bottom: 8px;
    }

    .feature-value {
        font-size: 2rem;
        font-weight: 900;
        color: white;
        letter-spacing: -0.03em;
    }

    .feature-caption {
        color: #7d98ae;
        font-size: 0.86rem;
        margin-top: 4px;
    }

    div[data-testid="stTabs"] button {
        border-radius: 999px;
        padding: 0.55rem 1.05rem;
        border: 1px solid rgba(0, 229, 255, 0.16);
        background: rgba(8, 21, 39, 0.65);
        margin-right: 0.45rem;
    }

    div[data-testid="stTabs"] button[aria-selected="true"] {
        color: white;
        border-color: rgba(0, 229, 255, 0.55);
        background: linear-gradient(90deg, rgba(0, 180, 255, .25), rgba(61, 124, 255, .23));
        box-shadow: 0 0 24px rgba(0, 229, 255, 0.10);
    }

    div[data-testid="stFileUploader"] {
        border-radius: 18px;
        border: 1px dashed rgba(0, 229, 255, 0.35);
        background: rgba(7, 23, 43, 0.45);
        padding: 8px;
    }

    .stButton > button, .stDownloadButton > button {
        border: 1px solid rgba(0, 229, 255, 0.45) !important;
        border-radius: 12px !important;
        background: linear-gradient(90deg, #087eff, #00bde8) !important;
        color: white !important;
        font-weight: 800 !important;
        box-shadow: 0 0 22px rgba(0, 174, 255, 0.18);
    }

    .stButton > button:hover, .stDownloadButton > button:hover {
        border-color: #7ff4ff !important;
        box-shadow: 0 0 30px rgba(0, 229, 255, 0.30);
        transform: translateY(-1px);
    }

    div[data-testid="stMetric"] {
        border: 1px solid rgba(0, 229, 255, 0.18);
        background: rgba(6, 18, 34, 0.72);
        padding: 14px 16px;
        border-radius: 16px;
    }

    .stem-header {
        border-left: 3px solid #00e5ff;
        padding-left: 12px;
        margin: 10px 0 16px;
    }

    .tiny-muted { color: #7895aa; font-size: .86rem; }
    </style>
    """,
    unsafe_allow_html=True,
)

st.markdown(
    """
    <div class="artist-hero">
        <div class="neon-pill">ArtistStudio • Creative AI Suite</div>
        <div class="brand">ARTISTSTUDIO</div>
        <div class="brand-sub">
            AI tools for producers, creators and artists — video generation, image animation,
            track intelligence and neural stem separation in one workspace.
        </div>
    </div>
    """,
    unsafe_allow_html=True,
)


def setup_replicate():
    token = None
    try:
        token = st.secrets.get("REPLICATE_API_TOKEN")
    except Exception:
        token = None
    token = token or os.getenv("REPLICATE_API_TOKEN")
    if token:
        os.environ["REPLICATE_API_TOKEN"] = token
        return True
    return False


REPLICATE_READY = setup_replicate()


def analyze_track(file_bytes):
    audio, sr = librosa.load(io.BytesIO(file_bytes), sr=None, mono=True)
    duration = librosa.get_duration(y=audio, sr=sr)

    tempo, _ = librosa.beat.beat_track(y=audio, sr=sr)
    tempo_value = float(np.asarray(tempo).reshape(-1)[0])

    chroma = librosa.feature.chroma_cqt(y=audio, sr=sr)
    chroma_mean = np.mean(chroma, axis=1)
    if np.sum(chroma_mean) > 0:
        chroma_mean = chroma_mean / np.sum(chroma_mean)

    major_profile = np.array([6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88])
    minor_profile = np.array([6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17])

    scores = []
    for root in range(12):
        scores.append(("major", root, np.corrcoef(chroma_mean, np.roll(major_profile, root))[0, 1]))
        scores.append(("minor", root, np.corrcoef(chroma_mean, np.roll(minor_profile, root))[0, 1]))

    mode, root, key_score = max(scores, key=lambda item: item[2])
    note_names = ["C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"]
    key_name = f"{note_names[root]} {'Major' if mode == 'major' else 'Minor'}"

    finite_scores = np.array([x[2] for x in scores if np.isfinite(x[2])])
    confidence = 0
    if finite_scores.size > 1 and np.isfinite(key_score):
        confidence = int(np.clip((key_score - np.median(finite_scores)) / 0.65 * 100, 1, 99))

    return {
        "bpm": round(tempo_value, 1),
        "key": key_name,
        "key_confidence": confidence,
        "duration": duration,
        "sample_rate": int(sr),
    }


def format_duration(seconds):
    seconds = max(0, int(round(seconds)))
    return f"{seconds // 60}:{seconds % 60:02d}"


def output_to_bytes(value):
    if value is None:
        return None
    if hasattr(value, "read"):
        try:
            return value.read()
        except Exception:
            pass
    response = requests.get(str(value), timeout=120)
    response.raise_for_status()
    return response.content


def separate_stems(file_bytes, filename, model_name="htdemucs"):
    if not REPLICATE_READY:
        raise RuntimeError("REPLICATE_API_TOKEN is not configured.")

    audio_input = io.BytesIO(file_bytes)
    audio_input.name = filename

    result = replicate.run(
        "cjwbw/demucs",
        input={
            "audio": audio_input,
            "model_name": model_name,
            "output_format": "wav",
            "shifts": 1,
            "overlap": 0.25,
            "clip_mode": "rescale",
        },
    )

    stem_bytes = {}
    for stem in ["vocals", "drums", "bass", "other", "guitar", "piano"]:
        value = result.get(stem) if isinstance(result, dict) else getattr(result, stem, None)
        if value:
            try:
                stem_bytes[stem] = output_to_bytes(value)
            except Exception:
                stem_bytes[stem] = None
    return {k: v for k, v in stem_bytes.items() if v}


def build_stem_zip(stems):
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED) as archive:
        for stem_name, data in stems.items():
            archive.writestr(f"ArtistStudio_{stem_name}.wav", data)
    return buffer.getvalue()


if "track_analysis" not in st.session_state:
    st.session_state.track_analysis = None
if "stem_results" not in st.session_state:
    st.session_state.stem_results = None
if "stem_source_name" not in st.session_state:
    st.session_state.stem_source_name = None


tab_stems, tab_promo, tab_image = st.tabs(["🎚️ Stem Lab", "🎬 AI Commercial", "🖼️ Image → Video"])

with tab_stems:
    st.markdown(
        """
        <div class="stem-header">
            <h2 style="margin-bottom:4px;">Neural Stem Lab</h2>
            <div class="tiny-muted">Analyze tempo and key, then separate a full mix into production-ready stems.</div>
        </div>
        """,
        unsafe_allow_html=True,
    )

    c1, c2, c3 = st.columns(3)
    with c1:
        st.markdown("""<div class="feature-card"><div class="feature-title">Track Intelligence</div><div class="feature-value">BPM + Key</div><div class="feature-caption">Instant tempo, musical key and track metadata.</div></div>""", unsafe_allow_html=True)
    with c2:
        st.markdown("""<div class="feature-card"><div class="feature-title">AI Separation</div><div class="feature-value">4 Stems</div><div class="feature-caption">Vocals, drums, bass and the remaining musical content.</div></div>""", unsafe_allow_html=True)
    with c3:
        st.markdown("""<div class="feature-card"><div class="feature-title">Producer Workflow</div><div class="feature-value">WAV Export</div><div class="feature-caption">Preview each result and download stems individually or together.</div></div>""", unsafe_allow_html=True)

    st.write("")
    uploaded_audio = st.file_uploader(
        "Drop a track here",
        type=["wav", "mp3", "flac", "m4a", "ogg"],
        help="For best results use a high-quality WAV or FLAC master.",
    )

    if uploaded_audio is not None:
        current_bytes = uploaded_audio.getvalue()
        source_changed = st.session_state.stem_source_name != uploaded_audio.name
        if source_changed:
            st.session_state.track_analysis = None
            st.session_state.stem_results = None
            st.session_state.stem_source_name = uploaded_audio.name

        st.audio(current_bytes)

        action1, action2, option_col = st.columns([1, 1, 1.2])
        with action1:
            analyze_clicked = st.button("⚡ Analyze Track", use_container_width=True)
        with action2:
            separate_clicked = st.button("✦ Separate Stems", use_container_width=True)
        with option_col:
            quality = st.selectbox(
                "Separation quality",
                ["Fast / High Quality", "Maximum Quality"],
                help="Maximum Quality uses the fine-tuned model and can take considerably longer.",
            )

        if analyze_clicked:
            with st.spinner("Scanning rhythm, tonal center and audio metadata..."):
                try:
                    st.session_state.track_analysis = analyze_track(current_bytes)
                except Exception as exc:
                    st.error(f"Track analysis failed: {exc}")

        if st.session_state.track_analysis:
            analysis = st.session_state.track_analysis
            m1, m2, m3, m4 = st.columns(4)
            m1.metric("BPM", analysis["bpm"])
            m2.metric("KEY", analysis["key"])
            m3.metric("LENGTH", format_duration(analysis["duration"]))
            m4.metric("SAMPLE RATE", f'{analysis["sample_rate"] / 1000:.1f} kHz')
            st.caption(f'Key estimate confidence: {analysis["key_confidence"]}%')

        if separate_clicked:
            if not REPLICATE_READY:
                st.error("Stem separation needs the existing REPLICATE_API_TOKEN configured in the app secrets.")
            else:
                if st.session_state.track_analysis is None:
                    with st.spinner("Analyzing track first..."):
                        try:
                            st.session_state.track_analysis = analyze_track(current_bytes)
                        except Exception:
                            pass

                selected_model = "htdemucs" if quality == "Fast / High Quality" else "htdemucs_ft"
                with st.status("Neural separation running…", expanded=True) as status:
                    try:
                        st.write("Uploading the mix to the separation engine…")
                        st.write("Extracting vocals, drums, bass and other musical content…")
                        stems = separate_stems(current_bytes, uploaded_audio.name, model_name=selected_model)
                        if not stems:
                            raise RuntimeError("The separation model returned no stem files.")
                        st.session_state.stem_results = stems
                        status.update(label="Stem separation complete", state="complete", expanded=False)
                    except Exception as exc:
                        status.update(label="Stem separation failed", state="error", expanded=True)
                        st.error(str(exc))

        if st.session_state.stem_results:
            st.markdown("### Separated stems")
            labels = {
                "vocals": "🎤 Vocals",
                "drums": "🥁 Drums",
                "bass": "🔊 Bass",
                "other": "🎹 Music / Other",
                "guitar": "🎸 Guitar",
                "piano": "🎹 Piano",
            }
            stems = st.session_state.stem_results
            for stem_name, stem_data in stems.items():
                st.markdown(f"#### {labels.get(stem_name, stem_name.title())}")
                st.audio(stem_data, format="audio/wav")
                st.download_button(
                    f"Download {stem_name.title()} WAV",
                    data=stem_data,
                    file_name=f"ArtistStudio_{stem_name}.wav",
                    mime="audio/wav",
                    key=f"download_{stem_name}",
                    use_container_width=True,
                )

            st.write("")
            st.download_button(
                "⬇ Download All Stems (.zip)",
                data=build_stem_zip(stems),
                file_name="ArtistStudio_StemLab.zip",
                mime="application/zip",
                use_container_width=True,
            )
    else:
        st.info("Upload a WAV, MP3, FLAC, M4A or OGG file to begin.")

with tab_promo:
    st.subheader("AI Commercial Maker")
    st.caption("Describe what you want to advertise. ArtistStudio writes the idea, voiceover and video direction.")

    user_idea = st.text_area(
        "What are you advertising?",
        placeholder="Example: A 20-second futuristic advert for my music production plugin.",
    )

    if st.button("Generate Smart Commercial", use_container_width=True, key="commercial_btn"):
        if not user_idea:
            st.warning("Enter an idea first.")
        elif not REPLICATE_READY:
            st.error("REPLICATE_API_TOKEN is not configured.")
        else:
            with st.status("Creating your commercial…", expanded=True) as status:
                try:
                    st.write("Writing script and directing the visual…")
                    brain_output = replicate.run(
                        "meta/meta-llama-3-70b-instruct",
                        input={
                            "prompt": f"I want an ad for: {user_idea}",
                            "system_prompt": "You are an expert commercial director. Write one detailed visual description for an AI video generator, then one short voiceover. Format exactly as VISUAL: <description> and SCRIPT: <voiceover>.",
                            "max_tokens": 200,
                        },
                    )
                    brain_text = "".join(brain_output)

                    visual_prompt = "A cinematic professional studio environment, 4K"
                    voice_script = "Create more with ArtistStudio."
                    for line in brain_text.split("\n"):
                        if line.startswith("VISUAL:"):
                            visual_prompt = line.replace("VISUAL:", "").strip()
                        elif line.startswith("SCRIPT:"):
                            voice_script = line.replace("SCRIPT:", "").strip()

                    st.info(f"🎥 **Visual direction:** {visual_prompt}")
                    st.info(f"🎙️ **Voiceover:** {voice_script}")

                    tts = gTTS(text=voice_script, lang="en", tld="com")
                    voice_buffer = io.BytesIO()
                    tts.write_to_fp(voice_buffer)
                    voice_buffer.seek(0)
                    st.audio(voice_buffer.getvalue(), format="audio/mp3")

                    st.write("Rendering cinematic footage…")
                    video_output = replicate.run("minimax/video-01", input={"prompt": visual_prompt})
                    vid_obj = video_output if not isinstance(video_output, list) else video_output[0]
                    try:
                        st.video(vid_obj.read())
                    except AttributeError:
                        st.video(str(vid_obj))

                    status.update(label="Commercial ready", state="complete", expanded=False)
                except Exception as exc:
                    status.update(label="Generation failed", state="error", expanded=True)
                    st.error(f"Something went wrong: {exc}")

with tab_image:
    st.subheader("Image → Video")
    st.caption("Upload a still image and turn it into a short animated clip.")

    uploaded_image = st.file_uploader("Upload image", type=["jpg", "jpeg", "png"], key="image_upload")

    if st.button("Animate Image", use_container_width=True, key="animate_btn"):
        if uploaded_image is None:
            st.warning("Upload a picture first.")
        elif not REPLICATE_READY:
            st.error("REPLICATE_API_TOKEN is not configured.")
        else:
            with st.spinner("Animating image…"):
                try:
                    image_input = io.BytesIO(uploaded_image.getvalue())
                    image_input.name = uploaded_image.name
                    output = replicate.run(
                        "stability-ai/stable-video-diffusion:3f0457e4619daac51203ddb4728167362e4f8c78c148386377e8a93a1523aef5",
                        input={"cond_image": image_input, "sizing_strategy": "maintain_aspect_ratio"},
                    )
                    video_output = output if not isinstance(output, list) else output[0]
                    try:
                        st.video(video_output.read())
                    except AttributeError:
                        st.video(str(video_output))
                    st.success("Image animation complete.")
                except Exception as exc:
                    st.error(f"Something went wrong: {exc}")
