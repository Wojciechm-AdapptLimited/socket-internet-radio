let audioContext, source;
let bufferIndex = 0;
let audioBuffers = [];

function init() {
    audioContext = new (window.AudioContext)();
    source = audioContext.createBufferSource();
    source.connect(audioContext.destination);
    for (let i = 0; i < 2; i++) {
        audioBuffers[i] = audioContext.createBuffer(1, 1024, audioContext.sampleRate);
    }
}

window.playAudio = function (audioData) {
    if (!audioContext) {
        init();
    }

    const buffer = audioBuffers[bufferIndex];
    const channelData = buffer.getChannelData(0);

    for (let i = 0; i < audioData.length; i++) {
        channelData[i] = audioData[i] / 128 - 1;
    }

    const source = audioContext.createBufferSource();
    source.buffer = buffer;
    source.connect(audioContext.destination);
    source.start();

    bufferIndex = (bufferIndex + 1) % 2;
}