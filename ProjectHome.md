This is a video transcoder project based on AVS.

The source codes include an optimized implementation of AVS encoding, an audio transcoder and a multi-thread implementation of transcoding. For the video part, the transcoder sequentially implements automatic decoding and encoding processes for each group of pictures(GOP). For the audio part, the transcoder first extracts the audio from original media file, and then it encodes the audio data with MP3 format.

The most time-consuming part of this AVS transcoder is the video encoding process which is the most critical part that we have and will make many possible efforts to optimize.

Welcome to use and join this opensource project!