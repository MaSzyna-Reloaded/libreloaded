@tool
extends AudioStream
class_name LazyAudioStream

## A stream that reads its data when it is played and can let it go - a sound out of reach needs
## none in memory (ScenerySoundServer). A plugin's stream of its data derives from it.


## Lets the stream's data go; it is read again when the stream is next played
func unload() -> void:
    pass
