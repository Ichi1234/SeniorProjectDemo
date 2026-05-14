# AiDosForge – AI Natural Language Appearance Tool

A UE5 plugin that converts natural language character descriptions into blend shape parameters on any skeletal mesh. Built as a senior capstone project at Kasetsart University, Department of Computer Engineering.

## Overview

AiDosForge takes a text prompt like *"tall muscular man with a square jaw"* and generates morph target values that are applied to a 3D character model in real time. The plugin uses prompt engineering over hosted LLMs — no custom model training required.

## How It Works

1. **Player enters a character description** in the in-game UI
2. **The plugin sends the description** to an LLM (Gemini 2.5 Flash Lite) along with a list of available morph targets and prompt rules
3. **The LLM returns a JSON object** mapping morph target names to float values (0.0–1.0)
4. **The plugin applies the values** to the skeletal mesh's blend shapes in UE5

## Supported Morph Target Categories

- **Head**: shape (round, square, oval, etc.), fat, age, scale
- **Face**: forehead, eyes, eyebrows, nose, mouth, chin/jaw, cheeks, ears
- **Neck**: height, width, circumference, double chin
- **Body**: height (torso + legs), shoulder width, waist, hips, bust
- **Muscle**: chest, back, arms, shoulders, forearms, thighs, calves
- **Fat**: arms, thighs, calves
- **Other**: stomach tone, buttocks, pelvis, breast

## Setup

### Prerequisites

- Unreal Engine 5.7
- Visual Studio 2022 (with C++ Desktop & Game Development workloads)
- .NET 8.0 SDK
- A Gemini API key (free at [Google AI Studio](https://aistudio.google.com/apikey))

### Installation

1. Clone this repository
2. If the project uses Git LFS: `git lfs install && git lfs pull`
3. Create a `.env` file in the project root:
   ```
   GEMINI_API_KEY=your_key_here
   ```
4. Open `SeniorProjectDemo.uproject` with UE5 5.7
5. Rebuild when prompted
6. Hit Play, type a description, click Generate

## Architecture

- **Inference backend**: Gemini 2.5 Flash Lite (via REST API)
- **Prompt engineering**: Structured prompt with morph target list, rules, value constraints, and few-shot example
- **Constrained output**: `responseMimeType: application/json` ensures valid JSON from Gemini
- **3D model**: MakeHuman mesh with 700+ blend shapes exported via Blender (MPFB)

## Prompt Engineering Experiment

A standalone Python CLI tool for experimenting with prompt engineering approaches for blend shape generation:

**[AIDosForgeExperiment](https://github.com/opxz7148/AIDosForgeExperiment)**

This experiment uses the Groq API with Llama 3.1 8B to test different prompt strategies (schema modes, few-shot counts, temperature settings) independently of the UE5 plugin.

## Team

- **Riccardo Mario Bonato** – Dataset curation & evaluation
- **Pasu Sangiemsin (Ichi)** – 3D pipeline & UE5 integration
- **Kasidet Uthaiwiwatkul (Opxzx)** – Prompt engineering & backend

**Advisor**: Dr. Chawanat Nakasan

## License

This project is part of a university capstone course and is intended for educational and research purposes.
