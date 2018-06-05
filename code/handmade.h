#if !defined(HANDMADE_H)

// NOTE: Services that the platform layer provides to the game

struct game_offscreen_buffer
{
    // NOTE: Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

/*
    @Params
    Timing
    ControllerInput
    Bitmap Buffer
    Sound Buffer

    @About: Services that the game provides to the platform layer.
    This may expand in the future - sound on a sperate thread, ...
*/
internal void GameUpdateAndRender(game_offscreen_buffer *Buffer);

#define HANDMADE_H
#endif