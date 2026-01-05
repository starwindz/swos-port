#include "ball.h"
#include "player.h"
#include "team.h"
#include "pitchConstants.h"
#include "comments.h"

using namespace SwosVM;

static void calculateNextBallPosition();
static void reverseDestXDirection();
static void reverseDestYDirection();

void updateBall()
{
    A0 = 328988;                            // mov A0, offset ballSprite
    ax = *(word *)&g_memByte[449800];       // mov ax, hideBall
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_show_ball;                   // jz short @@show_ball

    esi = A0;                               // mov esi, A0
    writeMemory(esi + 70, 2, -1);           // mov [esi+Sprite.imageIndex], -1
    *(word *)&g_memByte[329170] = -1;       // mov ballShadowSprite.imageIndex, -1
    goto l_calculate_deltas;                // jmp @@calculate_deltas

l_show_ball:;
    *(word *)&g_memByte[329170] = 1183;     // mov ballShadowSprite.imageIndex, SPR_BALL_SHADOW
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi + 46, 4);          // mov eax, [esi+Sprite.deltaX]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (!flags.zero)
        goto l_moving;                      // jnz short @@moving

    eax = readMemory(esi + 50, 4);          // mov eax, [esi+Sprite.deltaY]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (!flags.zero)
        goto l_moving;                      // jnz short @@moving

    {
        dword src = readMemory(esi + 18, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = 455972;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.frameIndicesTable], offset ballStaticFrameIndices
    if (flags.zero)
        goto l_set_ball_frame_index;        // jz short @@set_ball_frame_index

    writeMemory(esi + 18, 4, 455972);       // mov [esi+Sprite.frameIndicesTable], offset ballStaticFrameIndices
    writeMemory(esi + 22, 2, 0);            // mov [esi+Sprite.frameIndex], 0
    goto l_set_ball_frame_index;            // jmp short @@set_ball_frame_index

l_moving:;
    esi = A0;                               // mov esi, A0
    {
        dword src = readMemory(esi + 18, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = 455962;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.frameIndicesTable], offset ballMovingFrameIndices
    if (flags.zero)
        goto l_set_ball_frame_index;        // jz short @@set_ball_frame_index

    writeMemory(esi + 18, 4, 455962);       // mov [esi+Sprite.frameIndicesTable], offset ballMovingFrameIndices
    writeMemory(esi + 22, 2, 0);            // mov [esi+Sprite.frameIndex], 0

l_set_ball_frame_index:;
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_check_if_picture_set;        // jz short @@check_if_picture_set

    {
        word res = *(word *)&D0 >> 8;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 8
    {
        word res = *(word *)&D0 >> 1;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    ax = D0;                                // mov ax, word ptr D0
    {
        word src = (word)readMemory(esi + 26, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 26, 2, src);
    }                                       // sub [esi+Sprite.cycleFramesTimer], ax
    if (!flags.sign)
        goto l_check_if_picture_set;        // jns short @@check_if_picture_set

    {
        word src = (word)readMemory(esi + 22, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int16_t>(res)) & (srcSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 22, 2, src);
    }                                       // add [esi+Sprite.frameIndex], 1
    ax = (word)readMemory(esi + 24, 2);     // mov ax, [esi+Sprite.frameDelay]
    writeMemory(esi + 26, 2, ax);           // mov [esi+Sprite.cycleFramesTimer], ax
    goto l_extract_frame;                   // jmp short @@extract_frame

l_check_if_picture_set:;
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 70, 2);     // mov ax, [esi+Sprite.imageIndex]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.sign)
        goto l_calculate_deltas;            // jns short @@calculate_deltas

l_extract_frame:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi + 18, 4);          // mov eax, [esi+Sprite.frameIndicesTable]
    A1 = eax;                               // mov A1, eax
    ax = (word)readMemory(esi + 22, 2);     // mov ax, [esi+Sprite.frameIndex]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word res = *(word *)&D0 << 1;
        *(word *)&D0 = res;
    }                                       // shl word ptr D0, 1
    esi = A1;                               // mov esi, A1
    ebx = *(word *)&D0;                     // movzx ebx, word ptr D0
    ax = (word)readMemory(esi + ebx, 2);    // mov ax, [esi+ebx]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.sign)
        goto l_set_picture_index;           // jns short @@set_picture_index

    esi = A0;                               // mov esi, A0
    writeMemory(esi + 22, 2, 0);            // mov [esi+Sprite.frameIndex], 0
    goto l_extract_frame;                   // jmp short @@extract_frame

l_set_picture_index:;
    ax = D0;                                // mov ax, word ptr D0
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 70, 2, ax);           // mov [esi+Sprite.imageIndex], ax

l_calculate_deltas:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi + 30, 4);          // mov eax, [esi+Sprite.x]
    D5 = eax;                               // mov D5, eax
    eax = readMemory(esi + 34, 4);          // mov eax, [esi+Sprite.y]
    D6 = eax;                               // mov D6, eax
    eax = readMemory(esi + 38, 4);          // mov eax, [esi+Sprite.z]
    D7 = eax;                               // mov D7, eax
    ax = (word)readMemory(esi + 58, 2);     // mov ax, [esi+Sprite.destX]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = (word)readMemory(esi + 60, 2);     // mov ax, [esi+Sprite.destY]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    *(word *)&D3 = ax;                      // mov word ptr D3, ax
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    *(word *)&D4 = ax;                      // mov word ptr D4, ax
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    push(D5);                               // push D5
    push(D6);                               // push D6
    push(D7);                               // push D7
    push(A0);                               // push A0
    SWOS::CalculateDeltaXAndY();            // call CalculateDeltaXAndY
    pop(A0);                                // pop A0
    pop(D7);                                // pop D7
    pop(D6);                                // pop D6
    pop(D5);                                // pop D5
    eax = D1;                               // mov eax, D1
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 46, 4, eax);          // mov [esi+Sprite.deltaX], eax
    eax = D2;                               // mov eax, D2
    writeMemory(esi + 50, 4, eax);          // mov [esi+Sprite.deltaY], eax
    ax = D0;                                // mov ax, word ptr D0
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        goto l_set_direction;               // js short @@set_direction

    writeMemory(esi + 82, 2, ax);           // mov [esi+Sprite.fullDirection], ax
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 16;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 16
    {
        word res = *(word *)&D0 & 255;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 0FFh
    {
        word res = *(word *)&D0 >> 5;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 5

l_set_direction:;
    ax = D0;                                // mov ax, word ptr D0
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 42, 2, ax);           // mov [esi+Sprite.direction], ax
    ax = *(word *)&g_memByte[325628];       // mov ax, kBallGroundConstant
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = *(word *)&g_memByte[522832];       // mov ax, topTeamData.playerHasBall
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_check_z_coordinate;          // jnz short @@check_z_coordinate

    ax = *(word *)&g_memByte[522980];       // mov ax, bottomTeamData.playerHasBall
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_check_z_coordinate;          // jnz short @@check_z_coordinate

    ax = *(word *)&g_memByte[325644];       // mov ax, pitchBallSpeedFactor
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, ax

l_check_z_coordinate:;
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 40, 2);     // mov ax, word ptr [esi+(Sprite.z+2)]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_on_the_ground;               // jz short @@on_the_ground

    ax = *(word *)&g_memByte[325630];       // mov ax, kBallAirConstant
    *(word *)&D0 = ax;                      // mov word ptr D0, ax

l_on_the_ground:;
    ax = D0;                                // mov ax, word ptr D0
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 44, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 44, 2, src);
    }                                       // sub [esi+Sprite.speed], ax
    if (!flags.sign)
        goto l_apply_deltas;                // jns short @@apply_deltas

    writeMemory(esi + 44, 2, 0);            // mov [esi+Sprite.speed], 0

l_apply_deltas:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi + 46, 4);          // mov eax, [esi+Sprite.deltaX]
    D1 = eax;                               // mov D1, eax
    eax = readMemory(esi + 50, 4);          // mov eax, [esi+Sprite.deltaY]
    D2 = eax;                               // mov D2, eax
    eax = readMemory(esi + 54, 4);          // mov eax, [esi+Sprite.deltaZ]
    D3 = eax;                               // mov D3, eax
    eax = D1;                               // mov eax, D1
    {
        dword src = readMemory(esi + 30, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 30, 4, src);
    }                                       // add [esi+Sprite.x], eax
    eax = D2;                               // mov eax, D2
    {
        dword src = readMemory(esi + 34, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 34, 4, src);
    }                                       // add [esi+Sprite.y], eax
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (!flags.zero)
        goto l_keeper_doesnt_hold_the_ball; // jnz @@keeper_doesnt_hold_the_ball

    ax = *(word *)&g_memByte[486156];       // mov ax, g_substituteInProgress
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_keeper_is_controlled;        // jz short @@keeper_is_controlled

    eax = *(dword *)&g_memByte[486164];     // mov eax, teamThatSubstitutes
    A1 = eax;                               // mov A1, eax
    eax = *(dword *)&g_memByte[523108];     // mov eax, lastTeamPlayedBeforeBreak
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A1, eax
    if (!flags.zero)
        goto l_keeper_is_controlled;        // jnz short @@keeper_is_controlled

    esi = A1;                               // mov esi, A1
    eax = readMemory(esi + 32, 4);          // mov eax, [esi+TeamGeneralInfo.controlledPlayer]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (!flags.zero)
        goto l_keeper_is_controlled;        // jnz short @@keeper_is_controlled

    esi = A0;                               // mov esi, A0
    writeMemory(esi + 40, 2, 0);            // mov word ptr [esi+(Sprite.z+2)], 0
    goto l_set_delta_z_to_0;                // jmp @@set_delta_z_to_0

l_keeper_is_controlled:;
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_check_keeper_z;              // jz @@check_keeper_z

    {
        word src = *(word *)&g_memByte[455948];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int16_t>(res)) & (srcSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[455948] = src;
    }                                       // add writeOnlyVar04, 1
    eax = *(dword *)&g_memByte[523108];     // mov eax, lastTeamPlayedBeforeBreak
    A1 = eax;                               // mov A1, eax
    esi = A1;                               // mov esi, A1
    eax = readMemory(esi + 32, 4);          // mov eax, [esi+TeamGeneralInfo.controlledPlayer]
    A1 = eax;                               // mov A1, eax
    push(D0);                               // push D0
    push(D1);                               // push D1
    push(D2);                               // push D2
    push(D3);                               // push D3
    push(D4);                               // push D4
    push(D5);                               // push D5
    push(D6);                               // push D6
    push(D7);                               // push D7
    push(A0);                               // push A0
    push(A1);                               // push A1
    push(A2);                               // push A2
    push(A3);                               // push A3
    push(A4);                               // push A4
    push(A5);                               // push A5
    push(A6);                               // push A6
    updateBallWithControllingGoalkeeper();  // call UpdateBallWithControllingGoalkeeper
    pop(A6);                                // pop A6
    pop(A5);                                // pop A5
    pop(A4);                                // pop A4
    pop(A3);                                // pop A3
    pop(A2);                                // pop A2
    pop(A1);                                // pop A1
    pop(A0);                                // pop A0
    pop(D7);                                // pop D7
    pop(D6);                                // pop D6
    pop(D5);                                // pop D5
    pop(D4);                                // pop D4
    pop(D3);                                // pop D3
    pop(D2);                                // pop D2
    pop(D1);                                // pop D1
    pop(D0);                                // pop D0

l_check_keeper_z:;
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 40, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 5;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.z+2)], 5
    if (flags.zero)
        goto l_set_delta_z_to_0;            // jz @@set_delta_z_to_0

    if (!flags.carry && !flags.zero)
        goto l_height_higher_than_5;        // ja short @@height_higher_than_5

    D3 = 131076;                            // mov D3, 20004h
    goto l_apply_delta_z;                   // jmp short @@apply_delta_z

l_height_higher_than_5:;
    D3 = -65538;                            // mov D3, -10002h
    goto l_apply_delta_z;                   // jmp short @@apply_delta_z

l_keeper_doesnt_hold_the_ball:;
    eax = D3;                               // mov eax, D3
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (flags.zero)
        goto l_assign_delta_z;              // jz @@assign_delta_z

    eax = *(dword *)&g_memByte[325648];     // mov eax, kGravityConstant
    {
        int32_t dstSigned = D3;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        D3 = res;
    }                                       // sub D3, eax
    D3 |= 1;                                // or D3, 1

l_apply_delta_z:;
    eax = D3;                               // mov eax, D3
    esi = A0;                               // mov esi, A0
    {
        dword src = readMemory(esi + 38, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int32_t>(res)) & (srcSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 38, 4, src);
    }                                       // add [esi+Sprite.z], eax
    if (!flags.sign)
        goto l_assign_delta_z;              // jns @@assign_delta_z

    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    bx = *(word *)&g_memByte[325640];       // mov bx, ballSpeedBounceFactor
    tmp = ax * bx;
    ax = tmp.lo16;
    dx = tmp.hi16;                          // mul bx
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    *(word *)((byte *)&D0 + 2) = dx;        // mov word ptr D0+2, dx
    {
        dword res = D0 >> 8;
        D0 = res;
    }                                       // shr D0, 8
    ax = D0;                                // mov ax, word ptr D0
    {
        word src = (word)readMemory(esi + 44, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        src = res;
        writeMemory(esi + 44, 2, src);
    }                                       // sub [esi+Sprite.speed], ax
    writeMemory(esi + 40, 2, 0);            // mov word ptr [esi+(Sprite.z+2)], 0
    *(int32_t *)&D3 = -*(int32_t *)&D3;     // neg D3
    eax = D3;                               // mov eax, D3
    D0 = eax;                               // mov D0, eax
    {
        int32_t res = *(int32_t *)&D0 >> 8;
        *(int32_t *)&D0 = res;
    }                                       // sar D0, 8
    ax = D0;                                // mov ax, word ptr D0
    bx = *(word *)&g_memByte[325642];       // mov bx, ballBounceFactor
    tmp = ax * bx;
    ax = tmp.lo16;
    dx = tmp.hi16;                          // mul bx
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    *(word *)((byte *)&D0 + 2) = dx;        // mov word ptr D0+2, dx
    eax = D0;                               // mov eax, D0
    {
        int32_t dstSigned = D3;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        D3 = res;
    }                                       // sub D3, eax
    D3 |= 1;                                // or D3, 1
    {
        int32_t dstSigned = D3;
        int32_t srcSigned = 40960;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp D3, 0A000h
    if (!flags.carry && !flags.zero)
        goto l_play_ball_bounce_sample;     // ja short @@play_ball_bounce_sample

l_set_delta_z_to_0:;
    D3 = 0;                                 // mov D3, 0
    goto l_assign_delta_z;                  // jmp short @@assign_delta_z

l_play_ball_bounce_sample:;
    eax = D3;                               // mov eax, D3
    D0 = eax;                               // mov D0, eax
    push(D3);                               // push D3
    push(D5);                               // push D5
    push(D6);                               // push D6
    push(D7);                               // push D7
    push(A0);                               // push A0
    SWOS::PlayBallBounceSample();           // call PlayBallBounceSample
    pop(A0);                                // pop A0
    pop(D7);                                // pop D7
    pop(D6);                                // pop D6
    pop(D5);                                // pop D5
    pop(D3);                                // pop D3

l_assign_delta_z:;
    eax = D3;                               // mov eax, D3
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 54, 4, eax);          // mov [esi+Sprite.deltaZ], eax
    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 100;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, ST_GAME_IN_PROGRESS
    if (flags.zero)
        goto l_in_allowed_range_y;          // jz @@in_allowed_range_y

    {
        word src = (word)readMemory(esi + 32, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 53;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.x+2)], 53
    if (flags.sign != flags.overflow)
        goto l_bounce_off_invisible_barrier_x; // jl short @@bounce_off_invisible_barrier_x

    {
        word src = (word)readMemory(esi + 32, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 618;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.x+2)], 618
    if (flags.zero || flags.sign != flags.overflow)
        goto l_check_y_range;               // jle @@check_y_range

l_bounce_off_invisible_barrier_x:;
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 58, 2);     // mov ax, [esi+Sprite.destX]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = D0;                                // mov ax, word ptr D0
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D1 = res;
    }                                       // sub word ptr D1, ax
    ax = D1;                                // mov ax, word ptr D1
    writeMemory(esi + 58, 2, ax);           // mov [esi+Sprite.destX], ax
    {
        word src = (word)readMemory(esi + 44, 2);
        word res = src >> 1;
        src = res;
        writeMemory(esi + 44, 2, src);
    }                                       // shr [esi+Sprite.speed], 1
    eax = D5;                               // mov eax, D5
    writeMemory(esi + 30, 4, eax);          // mov [esi+Sprite.x], eax
    eax = D6;                               // mov eax, D6
    writeMemory(esi + 34, 4, eax);          // mov [esi+Sprite.y], eax
    eax = D7;                               // mov eax, D7
    writeMemory(esi + 38, 4, eax);          // mov [esi+Sprite.z], eax

l_check_y_range:;
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 100;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.y+2)], 100
    if (flags.sign != flags.overflow)
        goto l_bounce_off_invisible_barrier_y; // jl short @@bounce_off_invisible_barrier_y

    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 799;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.y+2)], 799
    if (flags.zero || flags.sign != flags.overflow)
        goto l_in_allowed_range_y;          // jle @@in_allowed_range_y

l_bounce_off_invisible_barrier_y:;
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 60, 2);     // mov ax, [esi+Sprite.destY]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = D0;                                // mov ax, word ptr D0
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D1 = res;
    }                                       // sub word ptr D1, ax
    ax = D1;                                // mov ax, word ptr D1
    writeMemory(esi + 60, 2, ax);           // mov [esi+Sprite.destY], ax
    {
        word src = (word)readMemory(esi + 44, 2);
        word res = src >> 1;
        src = res;
        writeMemory(esi + 44, 2, src);
    }                                       // shr [esi+Sprite.speed], 1
    eax = D5;                               // mov eax, D5
    writeMemory(esi + 30, 4, eax);          // mov [esi+Sprite.x], eax
    eax = D6;                               // mov eax, D6
    writeMemory(esi + 34, 4, eax);          // mov [esi+Sprite.y], eax
    eax = D7;                               // mov eax, D7
    writeMemory(esi + 38, 4, eax);          // mov [esi+Sprite.z], eax

l_in_allowed_range_y:;
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 129;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.y+2)], 129
    if (flags.sign != flags.overflow)
        goto l_goal_or_gol_out;             // jl short @@goal_or_gol_out

    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 769;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.y+2)], 769
    if (flags.zero || flags.sign != flags.overflow)
        goto l_not_in_lower_goal;           // jle @@not_in_lower_goal

l_goal_or_gol_out:;
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    ax = (word)readMemory(esi + 40, 2);     // mov ax, word ptr [esi+(Sprite.z+2)]
    *(word *)&D3 = ax;                      // mov word ptr D3, ax
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        *(word *)&D1 = res;
    }                                       // sub word ptr D1, 1
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 128;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 128
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_not_in_upper_goal;           // jg short @@not_in_upper_goal

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 112;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 112
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_in_upper_goal_y;             // jg @@in_upper_goal_y

l_not_in_upper_goal:;
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 770;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 770
    if (flags.sign != flags.overflow)
        goto l_not_in_lower_goal;           // jl @@not_in_lower_goal

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 785;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 785
    if (flags.sign == flags.overflow)
        goto l_not_in_lower_goal;           // jge @@not_in_lower_goal

    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 19;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 19
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_not_in_lower_goal;           // jg @@not_in_lower_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 295;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 295
    if (flags.zero || flags.sign != flags.overflow)
        goto l_not_in_lower_goal;           // jle @@not_in_lower_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 372;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 372
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_not_in_lower_goal;           // jg @@not_in_lower_goal

    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 15;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 15
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_ball_in_top_of_lower_goal;   // jg @@ball_in_top_of_lower_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 302;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 302
    if (flags.sign != flags.overflow)
        goto l_left_edge_of_lower_goal;     // jl @@left_edge_of_lower_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 366;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 366
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_left_edge_of_lower_goal;     // jg @@left_edge_of_lower_goal

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 778;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 778
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_ball_in_net;                 // jg @@ball_in_net

    goto l_not_in_lower_goal;               // jmp @@not_in_lower_goal

l_in_upper_goal_y:;
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 19;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 19
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_not_in_lower_goal;           // jg @@not_in_lower_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 295;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 295
    if (flags.zero || flags.sign != flags.overflow)
        goto l_not_in_lower_goal;           // jle @@not_in_lower_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 372;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 372
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_not_in_lower_goal;           // jg @@not_in_lower_goal

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 123;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 123
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_ball_just_in_upper_goal;     // jg short @@ball_just_in_upper_goal

    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 10;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 10
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_top_of_upper_goal;           // jg short @@top_of_upper_goal

    goto l_ball_in_upper_net;               // jmp short @@ball_in_upper_net

l_ball_just_in_upper_goal:;
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 15;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 15
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_top_of_upper_goal;           // jg short @@top_of_upper_goal

l_ball_in_upper_net:;
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 302;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 302
    if (flags.sign != flags.overflow)
        goto l_left_edge_of_lower_goal;     // jl @@left_edge_of_lower_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 366;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 366
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_left_edge_of_lower_goal;     // jg @@left_edge_of_lower_goal

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 119;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 119
    if (flags.sign != flags.overflow)
        goto l_ball_in_net;                 // jl @@ball_in_net

    goto l_not_in_lower_goal;               // jmp @@not_in_lower_goal

l_top_of_upper_goal:;
    eax = D7;                               // mov eax, D7
    D0 = eax;                               // mov D0, eax
    {
        word tmp = *(word *)((byte *)&D0 + 2);
        *(word *)((byte *)&D0 + 2) = ax;
        ax = tmp;
    }                                       // xchg ax, word ptr D0+2
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 15;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 15
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_top_of_the_goal;             // jg short @@top_of_the_goal

    esi = A0;                               // mov esi, A0
    writeMemory(esi + 54, 4, 1);            // mov [esi+Sprite.deltaZ], 1
    writeMemory(esi + 44, 2, 0);            // mov [esi+Sprite.speed], 0
    eax = D5;                               // mov eax, D5
    writeMemory(esi + 30, 4, eax);          // mov [esi+Sprite.x], eax
    eax = D6;                               // mov eax, D6
    writeMemory(esi + 34, 4, eax);          // mov [esi+Sprite.y], eax
    eax = D7;                               // mov eax, D7
    writeMemory(esi + 38, 4, eax);          // mov [esi+Sprite.z], eax
    goto l_not_in_lower_goal;               // jmp @@not_in_lower_goal

l_top_of_the_goal:;
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 54, 4, 1);            // mov [esi+Sprite.deltaZ], 1
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1000;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, 1000
    ax = D0;                                // mov ax, word ptr D0
    writeMemory(esi + 60, 2, ax);           // mov [esi+Sprite.destY], ax
    writeMemory(esi + 44, 2, 512);          // mov [esi+Sprite.speed], 512
    eax = D7;                               // mov eax, D7
    writeMemory(esi + 38, 4, eax);          // mov [esi+Sprite.z], eax
    goto l_not_in_lower_goal;               // jmp @@not_in_lower_goal

l_ball_in_top_of_lower_goal:;
    eax = D7;                               // mov eax, D7
    D0 = eax;                               // mov D0, eax
    {
        word tmp = *(word *)((byte *)&D0 + 2);
        *(word *)((byte *)&D0 + 2) = ax;
        ax = tmp;
    }                                       // xchg ax, word ptr D0+2
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 15;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 15
    if (!flags.zero && flags.sign == flags.overflow)
        goto cseg_7C69B;                    // jg short cseg_7C69B

    esi = A0;                               // mov esi, A0
    writeMemory(esi + 54, 4, 1);            // mov [esi+Sprite.deltaZ], 1
    writeMemory(esi + 44, 2, 0);            // mov [esi+Sprite.speed], 0
    eax = D5;                               // mov eax, D5
    writeMemory(esi + 30, 4, eax);          // mov [esi+Sprite.x], eax
    eax = D6;                               // mov eax, D6
    writeMemory(esi + 34, 4, eax);          // mov [esi+Sprite.y], eax
    eax = D7;                               // mov eax, D7
    writeMemory(esi + 38, 4, eax);          // mov [esi+Sprite.z], eax
    goto l_not_in_lower_goal;               // jmp @@not_in_lower_goal

cseg_7C69B:;
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 54, 4, 1);            // mov [esi+Sprite.deltaZ], 1
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1000;
        word res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int16_t>(res)) & (srcSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1000
    ax = D0;                                // mov ax, word ptr D0
    writeMemory(esi + 60, 2, ax);           // mov [esi+Sprite.destY], ax
    writeMemory(esi + 44, 2, 512);          // mov [esi+Sprite.speed], 512
    eax = D7;                               // mov eax, D7
    writeMemory(esi + 38, 4, eax);          // mov [esi+Sprite.z], eax
    goto l_not_in_lower_goal;               // jmp @@not_in_lower_goal

l_ball_in_net:;
    reverseDestYDirection();                // call ReverseDestYDirection
    resetBothTeamSpinTimers();              // call ResetBothTeamSpinTimers
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word res = *(word *)&D0 >> 3;
        flags.carry = ((word)*(word *)&D0 >> 13) & 1;
        *(word *)&D0 = res;
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // shr word ptr D0, 3
    ax = D0;                                // mov ax, word ptr D0
    writeMemory(esi + 44, 2, ax);           // mov [esi+Sprite.speed], ax
    goto cseg_7C756;                        // jmp short cseg_7C756

l_left_edge_of_lower_goal:;
    reverseDestXDirection();                // call ReverseDestXDirection
    resetBothTeamSpinTimers();              // call ResetBothTeamSpinTimers
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word res = *(word *)&D0 >> 2;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 2
    ax = D0;                                // mov ax, word ptr D0
    writeMemory(esi + 44, 2, ax);           // mov [esi+Sprite.speed], ax

cseg_7C756:;
    eax = D5;                               // mov eax, D5
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 30, 4, eax);          // mov [esi+Sprite.x], eax
    eax = D6;                               // mov eax, D6
    writeMemory(esi + 34, 4, eax);          // mov [esi+Sprite.y], eax

l_not_in_lower_goal:;
    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 100;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, ST_GAME_IN_PROGRESS
    if (!flags.zero)
        goto cseg_7CA2C;                    // jnz cseg_7CA2C

    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    ax = (word)readMemory(esi + 40, 2);     // mov ax, word ptr [esi+(Sprite.z+2)]
    *(word *)&D3 = ax;                      // mov word ptr D3, ax
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        *(word *)&D1 = res;
    }                                       // sub word ptr D1, 1
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 128;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 128
    if (flags.zero || flags.sign != flags.overflow)
        goto cseg_7CA2C;                    // jle cseg_7CA2C

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 132;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 132
    if (flags.zero || flags.sign != flags.overflow)
        goto cseg_7C7F0;                    // jle short cseg_7C7F0

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 770;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 770
    if (flags.sign == flags.overflow)
        goto cseg_7CA2C;                    // jge cseg_7CA2C

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 766;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 766
    if (flags.sign != flags.overflow)
        goto cseg_7CA2C;                    // jl cseg_7CA2C

cseg_7C7F0:;
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 19;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 19
    if (!flags.zero && flags.sign == flags.overflow)
        goto cseg_7CA2C;                    // jg cseg_7CA2C

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 295;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 295
    if (flags.zero || flags.sign != flags.overflow)
        goto cseg_7CA2C;                    // jle cseg_7CA2C

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 372;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 372
    if (!flags.zero && flags.sign == flags.overflow)
        goto cseg_7CA2C;                    // jg cseg_7CA2C

    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 15;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 15
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_penalty_goal;                // jg short @@penalty_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 302;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 302
    if (flags.sign != flags.overflow)
        goto l_own_goal;                    // jl short @@own_goal

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 366;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 366
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_own_goal;                    // jg short @@own_goal

    goto cseg_7CA2C;                        // jmp cseg_7CA2C

l_penalty_goal:;
    *(word *)&g_memByte[456248] = 1;        // mov goalTypeScored, GT_PENALTY
    ax = *(word *)&g_memByte[323904];       // mov ax, currentGameTick
    *(word *)&D4 = ax;                      // mov word ptr D4, ax
    {
        word res = *(word *)&D4 & 31;
        *(word *)&D4 = res;
    }                                       // and word ptr D4, 1Fh
    {
        word res = *(word *)&D4 << 4;
        *(word *)&D4 = res;
    }                                       // shl word ptr D4, 4
    {
        int16_t dstSigned = *(word *)&D4;
        int16_t srcSigned = 256;
        word res = dstSigned - srcSigned;
        *(word *)&D4 = res;
    }                                       // sub word ptr D4, 256
    esi = A0;                               // mov esi, A0
    {
        dword src = readMemory(esi + 50, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = -20480;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.deltaY], -5000h
    if (flags.sign != flags.overflow)
        goto cseg_7C911;                    // jl cseg_7C911

    {
        dword src = readMemory(esi + 50, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = 20480;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.deltaY], 5000h
    if (!flags.zero && flags.sign == flags.overflow)
        goto cseg_7C911;                    // jg cseg_7C911

    goto l_reverse_delta_z;                 // jmp @@reverse_delta_z

l_own_goal:;
    *(word *)&g_memByte[456248] = 2;        // mov goalTypeScored, GT_OWN_GOAL
    ax = *(word *)&g_memByte[323904];       // mov ax, currentGameTick
    *(word *)&D4 = ax;                      // mov word ptr D4, ax
    {
        word res = *(word *)&D4 & 31;
        *(word *)&D4 = res;
    }                                       // and word ptr D4, 1Fh
    {
        word res = *(word *)&D4 << 4;
        *(word *)&D4 = res;
    }                                       // shl word ptr D4, 4
    {
        int16_t dstSigned = *(word *)&D4;
        int16_t srcSigned = 256;
        word res = dstSigned - srcSigned;
        *(word *)&D4 = res;
    }                                       // sub word ptr D4, 256
    esi = A0;                               // mov esi, A0
    {
        dword src = readMemory(esi + 50, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = -20480;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.deltaY], -5000h
    if (flags.sign != flags.overflow)
        goto cseg_7C911;                    // jl short cseg_7C911

    {
        dword src = readMemory(esi + 50, 4);
        int32_t dstSigned = src;
        int32_t srcSigned = 20480;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.deltaY], 5000h
    if (!flags.zero && flags.sign == flags.overflow)
        goto cseg_7C911;                    // jg short cseg_7C911

    push(D4);                               // push D4
    reverseDestYDirection();                // call ReverseDestYDirection
    pop(D4);                                // pop D4
    resetBothTeamSpinTimers();              // call ResetBothTeamSpinTimers
    ax = D4;                                // mov ax, word ptr D4
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 60, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int16_t>(res)) & (srcSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 60, 2, src);
    }                                       // add [esi+Sprite.destY], ax
    goto cseg_7C989;                        // jmp short cseg_7C989

cseg_7C911:;
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 449;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 449
    if (!flags.zero && flags.sign == flags.overflow)
        goto cseg_7C92F;                    // jg short cseg_7C92F

    esi = A0;                               // mov esi, A0
    eax = readMemory(esi + 50, 4);          // mov eax, [esi+Sprite.deltaY]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (!flags.sign)
        goto cseg_7CA2C;                    // jns cseg_7CA2C

    goto cseg_7C940;                        // jmp short cseg_7C940

cseg_7C92F:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi + 50, 4);          // mov eax, [esi+Sprite.deltaY]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (flags.sign)
        goto cseg_7CA2C;                    // js cseg_7CA2C

cseg_7C940:;
    push(D4);                               // push D4
    reverseDestYDirection();                // call ReverseDestYDirection
    pop(D4);                                // pop D4
    resetBothTeamSpinTimers();              // call ResetBothTeamSpinTimers
    ax = D4;                                // mov ax, word ptr D4
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 58, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int16_t>(res)) & (srcSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 58, 2, src);
    }                                       // add [esi+Sprite.destX], ax
    goto cseg_7C989;                        // jmp short cseg_7C989

l_reverse_delta_z:;
    esi = A0;                               // mov esi, A0
    {
        int32_t src = readMemory(esi + 54, 4);
        src = -src;
        writeMemory(esi + 54, 4, src);
    }                                       // neg [esi+Sprite.deltaZ]
    eax = D7;                               // mov eax, D7
    writeMemory(esi + 38, 4, eax);          // mov [esi+Sprite.z], eax
    {
        int32_t dstSigned = D6;
        int32_t srcSigned = 65536;
        dword res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int32_t>(res)) & (srcSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
        D6 = res;
    }                                       // add D6, 10000h

cseg_7C989:;
    SWOS::Rand();                           // call Rand
    {
        word res = *(word *)&D0 & 1;
        flags.carry = false;
        flags.overflow = false;
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // test word ptr D0, 1
    if (flags.zero)
        goto l_play_frame_hit_comment;      // jz short @@play_frame_hit_comment

    {
        word src = *(word *)&g_memByte[456248];
        int16_t dstSigned = src;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp goalTypeScored, GT_OWN_GOAL
    if (!flags.zero)
        goto l_play_bar_hit_comment;        // jnz short @@play_bar_hit_comment

    push(A0);                               // push A0
    SWOS::PlayPostHitComment();             // call PlayPostHitComment
    pop(A0);                                // pop A0
    goto l_play_miss_goal_sample;           // jmp short @@play_miss_goal_sample

l_play_bar_hit_comment:;
    push(A0);                               // push A0
    SWOS::PlayBarHitComment();              // call PlayBarHitComment
    pop(A0);                                // pop A0
    goto l_play_miss_goal_sample;           // jmp short @@play_miss_goal_sample

l_play_frame_hit_comment:;
    push(A0);                               // push A0
    SWOS::PlayPostHitComment();             // call PlayPostHitComment
    pop(A0);                                // pop A0

l_play_miss_goal_sample:;
    SWOS::PlayMissGoalSample();             // call PlayMissGoalSample
    *(word *)&g_memByte[456248] = 0;        // mov goalTypeScored, GT_REGULAR
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word res = *(word *)&D0 >> 2;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 2
    ax = D0;                                // mov ax, word ptr D0
    {
        word src = (word)readMemory(esi + 44, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        src = res;
        writeMemory(esi + 44, 2, src);
    }                                       // sub [esi+Sprite.speed], ax
    eax = D5;                               // mov eax, D5
    writeMemory(esi + 30, 4, eax);          // mov [esi+Sprite.x], eax
    eax = D6;                               // mov eax, D6
    writeMemory(esi + 34, 4, eax);          // mov [esi+Sprite.y], eax

cseg_7CA2C:;
    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 100;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, ST_GAME_IN_PROGRESS
    if (!flags.zero)
        goto l_update_ball_shadow;          // jnz @@update_ball_shadow

    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 32, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 81;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.x+2)], 81
    if (flags.sign == flags.overflow)
        goto cseg_7CAA5;                    // jge short cseg_7CAA5

    push(D1);                               // push D1
    push(D2);                               // push D2
    push(D3);                               // push D3
    push(D4);                               // push D4
    push(D5);                               // push D5
    push(D6);                               // push D6
    push(A0);                               // push A0
    checkIfBallOutOfPlay();                 // call CheckIfBallOutOfPlay
    pop(A0);                                // pop A0
    pop(D6);                                // pop D6
    pop(D5);                                // pop D5
    pop(D4);                                // pop D4
    pop(D3);                                // pop D3
    pop(D2);                                // pop D2
    pop(D1);                                // pop D1
    goto l_update_ball_shadow;              // jmp @@update_ball_shadow

cseg_7CAA5:;
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 32, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 590;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.x+2)], 590
    if (flags.zero || flags.sign != flags.overflow)
        goto cseg_7CB11;                    // jle short cseg_7CB11

    push(D1);                               // push D1
    push(D2);                               // push D2
    push(D3);                               // push D3
    push(D4);                               // push D4
    push(D5);                               // push D5
    push(D6);                               // push D6
    push(A0);                               // push A0
    checkIfBallOutOfPlay();                 // call CheckIfBallOutOfPlay
    pop(A0);                                // pop A0
    pop(D6);                                // pop D6
    pop(D5);                                // pop D5
    pop(D4);                                // pop D4
    pop(D3);                                // pop D3
    pop(D2);                                // pop D2
    pop(D1);                                // pop D1
    goto l_update_ball_shadow;              // jmp @@update_ball_shadow

cseg_7CB11:;
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 129;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.y+2)], 129
    if (flags.sign == flags.overflow)
        goto cseg_7CB7A;                    // jge short cseg_7CB7A

    push(D1);                               // push D1
    push(D2);                               // push D2
    push(D3);                               // push D3
    push(D4);                               // push D4
    push(D5);                               // push D5
    push(D6);                               // push D6
    push(A0);                               // push A0
    checkIfBallOutOfPlay();                 // call CheckIfBallOutOfPlay
    pop(A0);                                // pop A0
    pop(D6);                                // pop D6
    pop(D5);                                // pop D5
    pop(D4);                                // pop D4
    pop(D3);                                // pop D3
    pop(D2);                                // pop D2
    pop(D1);                                // pop D1
    goto l_update_ball_shadow;              // jmp short @@update_ball_shadow

cseg_7CB7A:;
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 769;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr [esi+(Sprite.y+2)], 769
    if (flags.zero || flags.sign != flags.overflow)
        goto l_update_ball_shadow;          // jle short @@update_ball_shadow

    push(D1);                               // push D1
    push(D2);                               // push D2
    push(D3);                               // push D3
    push(D4);                               // push D4
    push(D5);                               // push D5
    push(D6);                               // push D6
    push(A0);                               // push A0
    checkIfBallOutOfPlay();                 // call CheckIfBallOutOfPlay
    pop(A0);                                // pop A0
    pop(D6);                                // pop D6
    pop(D5);                                // pop D5
    pop(D4);                                // pop D4
    pop(D3);                                // pop D3
    pop(D2);                                // pop D2
    pop(D1);                                // pop D1

l_update_ball_shadow:;
    A1 = 329100;                            // mov A1, offset ballShadowSprite
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 40, 2);     // mov ax, word ptr [esi+(Sprite.z+2)]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word res = *(word *)&D0 >> 1;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 1
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    esi = A1;                               // mov esi, A1
    writeMemory(esi + 32, 2, ax);           // mov word ptr [esi+(Sprite.x+2)], ax
    ax = D0;                                // mov ax, word ptr D0
    {
        word src = (word)readMemory(esi + 32, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 32, 2, src);
    }                                       // add word ptr [esi+(Sprite.x+2)], ax
    {
        word src = (word)readMemory(esi + 32, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 32, 2, src);
    }                                       // add word ptr [esi+(Sprite.x+2)], 1
    {
        word res = *(word *)&D0 >> 1;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 1
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    esi = A1;                               // mov esi, A1
    writeMemory(esi + 36, 2, ax);           // mov word ptr [esi+(Sprite.y+2)], ax
    ax = D0;                                // mov ax, word ptr D0
    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 36, 2, src);
    }                                       // add word ptr [esi+(Sprite.y+2)], ax
    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 36, 2, src);
    }                                       // add word ptr [esi+(Sprite.y+2)], 1
    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 10;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 36, 2, src);
    }                                       // sub word ptr [esi+(Sprite.y+2)], 10
    writeMemory(esi + 40, 2, -10);          // mov word ptr [esi+(Sprite.z+2)], -10
    calculateNextBallPosition();            // call CalculateNextBallPosition
    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 100;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, 100
    if (flags.zero)
        goto l_game_in_progress;            // jz short @@game_in_progress

    ax = *(word *)&g_memByte[523124];       // mov ax, foulXCoordinate
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = *(word *)&g_memByte[523126];       // mov ax, foulYCoordinate
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        goto l_keepers_ball_or_goal_out;    // jz short @@keepers_ball_or_goal_out

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_GOAL_OUT_LEFT
    if (flags.zero)
        goto l_keepers_ball_or_goal_out;    // jz short @@keepers_ball_or_goal_out

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_GOAL_OUT_RIGHT
    if (!flags.zero)
        goto l_calc_x_ball_quadrant;        // jnz short @@calc_x_ball_quadrant

l_keepers_ball_or_goal_out:;
    *(word *)&D1 = 336;                     // mov word ptr D1, 336
    *(word *)&D2 = 449;                     // mov word ptr D2, 449
    goto l_calc_x_ball_quadrant;            // jmp short @@calc_x_ball_quadrant

l_game_in_progress:;
    ax = *(word *)&g_memByte[523660];       // mov ax, ballNextX
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = *(word *)&g_memByte[523662];       // mov ax, ballNextY
    *(word *)&D2 = ax;                      // mov word ptr D2, ax

l_calc_x_ball_quadrant:;
    *(word *)&D3 = 0;                       // mov word ptr D3, 0
    *(word *)&D0 = 0;                       // mov word ptr D0, 0
    A1 = 523672;                            // mov A1, (offset ballXQuadrantLimits+2)
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, ax
    if (flags.carry)
        goto l_calc_y_ball_quadrant;        // jb @@calc_y_ball_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 1
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, ax
    if (flags.carry)
        goto l_calc_y_ball_quadrant;        // jb short @@calc_y_ball_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 1
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, ax
    if (flags.carry)
        goto l_calc_y_ball_quadrant;        // jb short @@calc_y_ball_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 1
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, ax
    if (flags.carry)
        goto l_calc_y_ball_quadrant;        // jb short @@calc_y_ball_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 1
    eax = A1;                               // mov eax, A1
    {
        int32_t dstSigned = eax;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        eax = res;
    }                                       // add eax, 2
    A1 = eax;                               // mov A1, eax

l_calc_y_ball_quadrant:;
    ax = D0;                                // mov ax, word ptr D0
    *(word *)&g_memByte[523758] = ax;       // mov ballXQuadrantDead, ax
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + -4, 2);     // mov ax, [esi-4]
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D1 = res;
    }                                       // sub word ptr D1, ax
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 51;
        word res = dstSigned - srcSigned;
        *(word *)&D1 = res;
    }                                       // sub word ptr D1, 51
    ax = D1;                                // mov ax, word ptr D1
    bx = 5;                                 // mov bx, 5
    {
        int32_t res = (int16_t)ax * (int16_t)bx;
        ax = res & 0xffff;
        dx = res >> 16;
    }                                       // imul bx
    bx = 15;                                // mov bx, 15
    {
        int32_t dividend = ((int16_t)dx << 16) | (int16_t)ax;
        int16_t quot = (int16_t)(dividend / (int16_t)bx);
        int16_t rem = (int16_t)(dividend % (int16_t)bx);
        ax = quot;
        dx = rem;
    }                                       // idiv bx
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    *(word *)((byte *)&D1 + 2) = dx;        // mov word ptr D1+2, dx
    *(word *)&g_memByte[523762] = ax;       // mov playerXQuadrantOffset, ax
    *(word *)&D0 = 0;                       // mov word ptr D0, 0
    A1 = 523682;                            // mov A1, (offset ballYQuadrantLimits+2)
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, ax
    if (flags.carry)
        goto l_set_y_quadrant;              // jb @@set_y_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 5;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 5
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, ax
    if (flags.carry)
        goto l_set_y_quadrant;              // jb @@set_y_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 5;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 5
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, ax
    if (flags.carry)
        goto l_set_y_quadrant;              // jb @@set_y_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 5;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 5
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, ax
    if (flags.carry)
        goto l_set_y_quadrant;              // jb short @@set_y_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 5;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 5
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, ax
    if (flags.carry)
        goto l_set_y_quadrant;              // jb short @@set_y_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 5;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 5
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, ax
    if (flags.carry)
        goto l_set_y_quadrant;              // jb short @@set_y_quadrant

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 5;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 5
    eax = A1;                               // mov eax, A1
    {
        int32_t dstSigned = eax;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        eax = res;
    }                                       // add eax, 2
    A1 = eax;                               // mov A1, eax

l_set_y_quadrant:;
    ax = D0;                                // mov ax, word ptr D0
    *(word *)&g_memByte[523760] = ax;       // mov ballYQuadrantDead, ax
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + -4, 2);     // mov ax, [esi-4]
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D2 = res;
    }                                       // sub word ptr D2, ax
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 45;
        word res = dstSigned - srcSigned;
        *(word *)&D2 = res;
    }                                       // sub word ptr D2, 45
    ax = D2;                                // mov ax, word ptr D2
    bx = 5;                                 // mov bx, 5
    {
        int32_t res = (int16_t)ax * (int16_t)bx;
        ax = res & 0xffff;
        dx = res >> 16;
    }                                       // imul bx
    bx = 15;                                // mov bx, 15
    {
        int32_t dividend = ((int16_t)dx << 16) | (int16_t)ax;
        int16_t quot = (int16_t)(dividend / (int16_t)bx);
        int16_t rem = (int16_t)(dividend % (int16_t)bx);
        ax = quot;
        dx = rem;
    }                                       // idiv bx
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    *(word *)((byte *)&D2 + 2) = dx;        // mov word ptr D2+2, dx
    *(word *)&g_memByte[523764] = ax;       // mov playerYQuadrantOffset, ax
    ax = D3;                                // mov ax, word ptr D3
    *(word *)&g_memByte[523756] = ax;       // mov ballQuadrantIndex, ax
}

// in:
//      A6 -> team data
//
// Apply "after effects" to pass/kick. Apply spin if player holding left/right
// directions, and high kick if player holding back direction. Also update ball
// for normal kicks too. Slow down ball after kick/pass.
//
void applyBallAfterTouch()
{
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 128, 2);    // mov ax, [esi+TeamGeneralInfo.passInProgress]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_passing_now;                 // jnz @@passing_now

    eax = readMemory(esi, 4);               // mov eax, [esi+TeamGeneralInfo.opponentsTeam]
    A0 = eax;                               // mov A0, eax
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 76, 2);     // mov ax, [esi+TeamGeneralInfo.goalkeeperSavedCommentTimer]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        goto l_reset_spin_timer_out;        // js @@reset_spin_timer_out

    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 100;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, ST_GAME_IN_PROGRESS
    if (flags.zero)
        goto l_game_in_progress;            // jz short @@game_in_progress

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        goto l_reset_spin_timer_out;        // jz @@reset_spin_timer_out

l_game_in_progress:;
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 118, 2);    // mov ax, [esi+TeamGeneralInfo.spinTimer]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        return;                             // js @@out

    if (!flags.zero)
        goto l_accepting_spin;              // jnz short @@accepting_spin

    writeMemory(esi + 120, 2, 0);           // mov [esi+TeamGeneralInfo.leftSpin], 0
    writeMemory(esi + 122, 2, 0);           // mov [esi+TeamGeneralInfo.rightSpin], 0

l_accepting_spin:;
    A1 = 328988;                            // mov A1, offset ballSprite
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 120, 2);    // mov ax, [esi+TeamGeneralInfo.leftSpin]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_already_spining_left;        // jnz @@already_spining_left

    ax = (word)readMemory(esi + 122, 2);    // mov ax, [esi+TeamGeneralInfo.rightSpin]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_already_spining_right;       // jnz short @@already_spining_right

    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+TeamGeneralInfo.currentAllowedDirection]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        goto l_not_starting_spin;           // js @@not_starting_spin

    ax = (word)readMemory(esi + 56, 2);     // mov ax, [esi+TeamGeneralInfo.controlledPlDirection]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = D1;                                // mov ax, word ptr D1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    if (flags.zero)
        goto l_not_starting_spin;           // jz @@not_starting_spin

    {
        word res = *(word *)&D0 & 7;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 7
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 4;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 4
    if (flags.zero)
        goto l_not_starting_spin;           // jz @@not_starting_spin

    if (flags.carry)
        goto l_spin_left;                   // jb short @@spin_left

    writeMemory(esi + 122, 2, 1);           // mov [esi+TeamGeneralInfo.rightSpin], 1

l_already_spining_right:;
    D1 = 4;                                 // mov D1, 4
    goto l_calculate_initial_spin;          // jmp short @@calculate_initial_spin

l_spin_left:;
    esi = A6;                               // mov esi, A6
    writeMemory(esi + 120, 2, 1);           // mov [esi+TeamGeneralInfo.leftSpin], 1

l_already_spining_left:;
    *(word *)&D1 = 0;                       // mov word ptr D1, 0

l_calculate_initial_spin:;
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 56, 2);     // mov ax, [esi+TeamGeneralInfo.controlledPlDirection]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    A0 = 325806;                            // mov A0, offset kKickSpinFactor
    {
        word res = *(word *)&D0 << 3;
        *(word *)&D0 = res;
    }                                       // shl word ptr D0, 3
    ax = D1;                                // mov ax, word ptr D1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, ax
    A2 = 325786;                            // mov A2, offset kSpinMultiplierFactor
    ax = (word)readMemory(esi + 118, 2);    // mov ax, [esi+TeamGeneralInfo.spinTimer]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    {
        word res = *(word *)&D2 << 1;
        *(word *)&D2 = res;
    }                                       // shl word ptr D2, 1
    esi = A2;                               // mov esi, A2
    ebx = *(word *)&D2;                     // movzx ebx, word ptr D2
    ax = (word)readMemory(esi + ebx, 2);    // mov ax, [esi+ebx]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    esi = A0;                               // mov esi, A0
    ebx = *(word *)&D0;                     // movzx ebx, word ptr D0
    ax = (word)readMemory(esi + ebx, 2);    // mov ax, [esi+ebx]
    {
        int32_t res = (int16_t)ax * *(int16_t *)&D2;
        ax = res & 0xffff;
        dx = res >> 16;
    }                                       // imul word ptr D2
    esi = A1;                               // mov esi, A1
    {
        word src = (word)readMemory(esi + 58, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 58, 2, src);
    }                                       // add [esi+Sprite.destX], ax
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + ebx + 2, 2); // mov ax, [esi+ebx+2]
    {
        int32_t res = (int16_t)ax * *(int16_t *)&D2;
        ax = res & 0xffff;
        dx = res >> 16;
    }                                       // imul word ptr D2
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    *(word *)((byte *)&D1 + 2) = dx;        // mov word ptr D1+2, dx
    esi = A1;                               // mov esi, A1
    {
        word src = (word)readMemory(esi + 60, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 60, 2, src);
    }                                       // add [esi+Sprite.destY], ax

l_not_starting_spin:;
    esi = A6;                               // mov esi, A6
    {
        word src = (word)readMemory(esi + 118, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 4;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.spinTimer], 4
    if (!flags.zero)
        goto l_increase_spin_timer;         // jnz @@increase_spin_timer

    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+TeamGeneralInfo.currentAllowedDirection]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        goto l_normal_not_high_kick;        // js short @@normal_not_high_kick

    ax = (word)readMemory(esi + 56, 2);     // mov ax, [esi+TeamGeneralInfo.controlledPlDirection]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = D1;                                // mov ax, word ptr D1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    if (flags.zero)
        goto l_jmp_increase_spin_timer;     // jz short @@jmp_increase_spin_timer

    {
        word res = *(word *)&D0 & 7;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 7
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 2
    if (flags.zero)
        goto l_normal_not_high_kick;        // jz short @@normal_not_high_kick

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 6;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 6
    if (flags.zero)
        goto l_normal_not_high_kick;        // jz short @@normal_not_high_kick

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 3
    if (flags.carry)
        goto l_jmp_increase_spin_timer;     // jb short @@jmp_increase_spin_timer

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 5;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 5
    if (!flags.carry && !flags.zero)
        goto l_jmp_increase_spin_timer;     // ja short @@jmp_increase_spin_timer

    eax = *(dword *)&g_memByte[325774];     // mov eax, kHighKickDeltaZ
    esi = A1;                               // mov esi, A1
    writeMemory(esi + 54, 4, eax);          // mov [esi+Sprite.deltaZ], eax
    ax = *(word *)&g_memByte[325778];       // mov ax, kHighKickBallSpeed
    writeMemory(esi + 44, 2, ax);           // mov [esi+Sprite.speed], ax
    goto l_do_some_speed_adjustment;        // jmp short @@do_some_speed_adjustment

l_normal_not_high_kick:;
    eax = *(dword *)&g_memByte[325780];     // mov eax, kNormalKickDeltaZ
    esi = A1;                               // mov esi, A1
    writeMemory(esi + 54, 4, eax);          // mov [esi+Sprite.deltaZ], eax
    ax = *(word *)&g_memByte[325784];       // mov ax, kNormalKickBallSpeed
    writeMemory(esi + 44, 2, ax);           // mov [esi+Sprite.speed], ax
    goto l_do_some_speed_adjustment;        // jmp short @@do_some_speed_adjustment

l_jmp_increase_spin_timer:;
    goto l_increase_spin_timer;             // jmp @@increase_spin_timer

l_do_some_speed_adjustment:;
    esi = A6;                               // mov esi, A6
    {
        word src = (word)readMemory(esi + 56, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 0;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.controlledPlDirection], 0
    if (flags.zero)
        goto l_decrease_ball_speed_by_quarter; // jz @@decrease_ball_speed_by_quarter

    {
        word src = (word)readMemory(esi + 56, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 4;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.controlledPlDirection], 4
    if (flags.zero)
        goto l_decrease_ball_speed_by_quarter; // jz short @@decrease_ball_speed_by_quarter

    {
        byte src = (byte)readMemory(esi + 56, 1);
        byte res = src & 1;
        flags.carry = false;
        flags.sign = (res & 0x80) != 0;
        flags.zero = res == 0;
    }                                       // test byte ptr [esi+TeamGeneralInfo.controlledPlDirection], 1
    if (flags.zero)
        goto l_increase_spin_timer;         // jz @@increase_spin_timer

    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    {
        word res = *(word *)&D1 >> 2;
        *(word *)&D1 = res;
    }                                       // shr word ptr D1, 2
    *(int16_t *)&D1 = -*(int16_t *)&D1;     // neg word ptr D1
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        *(word *)&D1 = res;
    }                                       // add word ptr D1, ax
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    {
        word res = *(word *)&D2 >> 3;
        *(word *)&D2 = res;
    }                                       // shr word ptr D2, 3
    ax = D2;                                // mov ax, word ptr D2
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D1 = res;
    }                                       // add word ptr D1, ax
    ax = D1;                                // mov ax, word ptr D1
    writeMemory(esi + 44, 2, ax);           // mov [esi+Sprite.speed], ax
    goto l_increase_spin_timer;             // jmp short @@increase_spin_timer

l_decrease_ball_speed_by_quarter:;
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    {
        word res = *(word *)&D1 >> 2;
        *(word *)&D1 = res;
    }                                       // shr word ptr D1, 2
    *(int16_t *)&D1 = -*(int16_t *)&D1;     // neg word ptr D1
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        *(word *)&D1 = res;
    }                                       // add word ptr D1, ax
    ax = D1;                                // mov ax, word ptr D1
    writeMemory(esi + 44, 2, ax);           // mov [esi+Sprite.speed], ax

l_increase_spin_timer:;
    esi = A6;                               // mov esi, A6
    {
        word src = (word)readMemory(esi + 118, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 118, 2, src);
    }                                       // add [esi+TeamGeneralInfo.spinTimer], 1
    {
        word src = (word)readMemory(esi + 118, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 10;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.spinTimer], 10
    if (!flags.zero)
        return;                             // jnz short @@out

l_reset_spin_timer_out:;
    esi = A6;                               // mov esi, A6
    writeMemory(esi + 118, 2, -1);          // mov [esi+TeamGeneralInfo.spinTimer], -1

l_out:;
    return;                                 // retn

l_passing_now:;
    esi = A6;                               // mov esi, A6
    eax = readMemory(esi, 4);               // mov eax, [esi+TeamGeneralInfo.opponentsTeam]
    A0 = eax;                               // mov A0, eax
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 76, 2);     // mov ax, [esi+TeamGeneralInfo.goalkeeperSavedCommentTimer]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        goto l_out_reset_spin_timer;        // js @@out_reset_spin_timer

    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 100;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, 100
    if (flags.zero)
        goto l_game_in_progress2;           // jz short @@game_in_progress2

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        goto l_out_reset_spin_timer;        // jz @@out_reset_spin_timer

l_game_in_progress2:;
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 118, 2);    // mov ax, [esi+TeamGeneralInfo.spinTimer]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        return;                             // js @@out2

    if (!flags.zero)
        goto l_spin_timer_running;          // jnz short @@spin_timer_running

    writeMemory(esi + 120, 2, 0);           // mov [esi+TeamGeneralInfo.leftSpin], 0
    writeMemory(esi + 122, 2, 0);           // mov [esi+TeamGeneralInfo.rightSpin], 0
    writeMemory(esi + 124, 2, 0);           // mov [esi+TeamGeneralInfo.longPass], 0
    writeMemory(esi + 126, 2, 0);           // mov [esi+TeamGeneralInfo.longSpinPass], 0

l_spin_timer_running:;
    A1 = 328988;                            // mov A1, offset ballSprite
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 120, 2);    // mov ax, [esi+TeamGeneralInfo.leftSpin]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_pass_spining_left;           // jnz @@pass_spining_left

    ax = (word)readMemory(esi + 122, 2);    // mov ax, [esi+TeamGeneralInfo.rightSpin]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_pass_spining_right;          // jnz short @@pass_spining_right

    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+TeamGeneralInfo.currentAllowedDirection]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        goto l_no_spin_to_apply;            // js @@no_spin_to_apply

    ax = (word)readMemory(esi + 56, 2);     // mov ax, [esi+TeamGeneralInfo.controlledPlDirection]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = D1;                                // mov ax, word ptr D1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    if (flags.zero)
        goto l_no_spin_to_apply;            // jz @@no_spin_to_apply

    {
        word res = *(word *)&D0 & 7;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 7
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 4;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 4
    if (flags.zero)
        goto l_no_spin_to_apply;            // jz @@no_spin_to_apply

    if (flags.carry)
        goto l_turn_on_left_spin;           // jb short @@turn_on_left_spin

    writeMemory(esi + 122, 2, 1);           // mov [esi+TeamGeneralInfo.rightSpin], 1

l_pass_spining_right:;
    D1 = 4;                                 // mov D1, 4
    goto l_apply_spin_factor;               // jmp short @@apply_spin_factor

l_turn_on_left_spin:;
    esi = A6;                               // mov esi, A6
    writeMemory(esi + 120, 2, 1);           // mov [esi+TeamGeneralInfo.leftSpin], 1

l_pass_spining_left:;
    *(word *)&D1 = 0;                       // mov word ptr D1, 0

l_apply_spin_factor:;
    ax = *(word *)&g_memByte[329030];       // mov ax, ballSprite.direction
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    A0 = 325870;                            // mov A0, offset kPassingSpinFactor
    {
        word res = *(word *)&D0 << 3;
        *(word *)&D0 = res;
    }                                       // shl word ptr D0, 3
    ax = D1;                                // mov ax, word ptr D1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, ax
    A2 = 325786;                            // mov A2, offset kSpinMultiplierFactor
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 118, 2);    // mov ax, [esi+TeamGeneralInfo.spinTimer]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    {
        word res = *(word *)&D2 << 1;
        *(word *)&D2 = res;
    }                                       // shl word ptr D2, 1
    esi = A2;                               // mov esi, A2
    ebx = *(word *)&D2;                     // movzx ebx, word ptr D2
    ax = (word)readMemory(esi + ebx, 2);    // mov ax, [esi+ebx]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    esi = A0;                               // mov esi, A0
    ebx = *(word *)&D0;                     // movzx ebx, word ptr D0
    ax = (word)readMemory(esi + ebx, 2);    // mov ax, [esi+ebx]
    {
        int32_t res = (int16_t)ax * *(int16_t *)&D2;
        ax = res & 0xffff;
        dx = res >> 16;
    }                                       // imul word ptr D2
    esi = A1;                               // mov esi, A1
    {
        word src = (word)readMemory(esi + 58, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 58, 2, src);
    }                                       // add [esi+Sprite.destX], ax
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + ebx + 2, 2); // mov ax, [esi+ebx+2]
    {
        int32_t res = (int16_t)ax * *(int16_t *)&D2;
        ax = res & 0xffff;
        dx = res >> 16;
    }                                       // imul word ptr D2
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    *(word *)((byte *)&D1 + 2) = dx;        // mov word ptr D1+2, dx
    esi = A1;                               // mov esi, A1
    {
        word src = (word)readMemory(esi + 60, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 60, 2, src);
    }                                       // add [esi+Sprite.destY], ax

l_no_spin_to_apply:;
    esi = A6;                               // mov esi, A6
    eax = readMemory(esi + 124, 4);         // mov eax, dword ptr [esi+TeamGeneralInfo.longPass]
    flags.carry = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (!flags.zero)
        goto l_update_spin_timer;           // jnz @@update_spin_timer

    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+TeamGeneralInfo.currentAllowedDirection]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        goto l_holding_left_or_right;       // js @@holding_left_or_right

    ax = (word)readMemory(esi + 56, 2);     // mov ax, [esi+TeamGeneralInfo.controlledPlDirection]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = D1;                                // mov ax, word ptr D1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    if (flags.zero)
        goto l_update_spin_timer2;          // jz @@update_spin_timer2

    {
        word res = *(word *)&D0 & 7;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 7
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 2
    if (flags.zero)
        goto l_holding_left_or_right;       // jz short @@holding_left_or_right

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 6;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 6
    if (flags.zero)
        goto l_holding_left_or_right;       // jz short @@holding_left_or_right

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 3
    if (flags.carry)
        goto l_update_spin_timer2;          // jb short @@update_spin_timer2

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 5;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 5
    if (!flags.carry && !flags.zero)
        goto l_update_spin_timer2;          // ja short @@update_spin_timer2

    writeMemory(esi + 126, 2, 1);           // mov [esi+TeamGeneralInfo.longSpinPass], 1
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word res = *(word *)&D0 >> 3;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 3
    ax = D0;                                // mov ax, word ptr D0
    {
        word src = (word)readMemory(esi + 44, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 44, 2, src);
    }                                       // add [esi+Sprite.speed], ax
    goto l_update_spin_timer;               // jmp short @@update_spin_timer

l_holding_left_or_right:;
    esi = A6;                               // mov esi, A6
    writeMemory(esi + 124, 2, 1);           // mov [esi+TeamGeneralInfo.longPass], 1
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word res = *(word *)&D0 >> 3;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 3
    ax = D0;                                // mov ax, word ptr D0
    {
        word src = (word)readMemory(esi + 44, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 44, 2, src);
    }                                       // add [esi+Sprite.speed], ax
    goto l_update_spin_timer;               // jmp short @@update_spin_timer

l_update_spin_timer2:;

l_update_spin_timer:;
    esi = A6;                               // mov esi, A6
    {
        word src = (word)readMemory(esi + 118, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 118, 2, src);
    }                                       // add [esi+TeamGeneralInfo.spinTimer], 1
    {
        word src = (word)readMemory(esi + 118, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 10;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.spinTimer], 10
    if (!flags.zero)
        return;                             // jnz short @@out2

l_out_reset_spin_timer:;
    esi = A6;                               // mov esi, A6
    writeMemory(esi + 118, 2, -1);          // mov [esi+TeamGeneralInfo.spinTimer], -1
}

void checkIfBallOutOfPlay()
{
    *(word *)&g_memByte[456262] = 0;        // mov stateGoal, 0
    *(word *)&g_memByte[523668] = 0;        // mov playRefereeWhistle, 0
    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 101;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, ST_STOPPED
    if (flags.zero)
        goto l_starting_the_game;           // jz @@starting_the_game

    *(word *)&g_memByte[523668] = 1;        // mov playRefereeWhistle, 1
    A6 = 328988;                            // mov A6, offset ballSprite
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    ax = (word)readMemory(esi + 40, 2);     // mov ax, word ptr [esi+(Sprite.z+2)]
    *(word *)&D3 = ax;                      // mov word ptr D3, ax
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        *(word *)&D1 = res;
    }                                       // sub word ptr D1, 1
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 15;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 15
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_not_in_goal_check_near_miss; // jg @@not_in_goal_check_near_miss

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 302;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 302
    if (flags.sign != flags.overflow)
        goto l_not_in_goal_check_near_miss; // jl @@not_in_goal_check_near_miss

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 366;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 366
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_not_in_goal_check_near_miss; // jg @@not_in_goal_check_near_miss

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 449;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 449
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_lower_goal;                  // jg @@lower_goal

    A6 = 522940;                            // mov A6, offset bottomTeamData
    esi = A6;                               // mov esi, A6
    {
        word src = (word)readMemory(esi + 18, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.teamNumber], 1
    if (flags.zero)
        goto l_team1_scored;                // jz @@team1_scored

l_team2_scored:;
    *(word *)&D0 = 2;                       // mov word ptr D0, 2
    eax = *(dword *)&g_memByte[523096];     // mov eax, lastPlayerPlayed
    A0 = eax;                               // mov A0, eax
    eax = *(dword *)&g_memByte[523087];     // mov eax, lastKeeperPlayed
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (flags.zero)
        goto l_keeper_didnt_play;           // jz short @@keeper_didnt_play

    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 326524;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A0, offset goalie1Sprite
    if (flags.zero)
        goto l_keeper_played;               // jz short @@keeper_played

    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 327756;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A0, offset goalie2Sprite
    if (!flags.zero)
        goto l_keeper_didnt_play;           // jnz short @@keeper_didnt_play

l_keeper_played:;
    eax = *(dword *)&g_memByte[523087];     // mov eax, lastKeeperPlayed
    A0 = eax;                               // mov A0, eax

l_keeper_didnt_play:;
    ax = *(word *)&g_memByte[336636];       // mov ax, statsTeam1Goals
    *(word *)&g_memByte[336624] = ax;       // mov statsTeam1GoalsCopy2, ax
    ax = *(word *)&g_memByte[336638];       // mov ax, statsTeam2Goals
    *(word *)&g_memByte[336626] = ax;       // mov statsTeam2GoalsCopy2, ax
    push(A6);                               // push A6
    SWOS::GoalScored();                     // call GoalScored
    pop(A6);                                // pop A6
    *(word *)&g_memByte[455982] = 1;        // mov goalCameraMode, 1
    *(word *)&g_memByte[456644] = 2;        // mov teamNumThatScored, 2
    goto l_goal_handled;                    // jmp @@goal_handled

l_lower_goal:;
    A6 = 522792;                            // mov A6, offset topTeamData
    esi = A6;                               // mov esi, A6
    {
        word src = (word)readMemory(esi + 18, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.teamNumber], 2
    if (flags.zero)
        goto l_team2_scored;                // jz @@team2_scored

l_team1_scored:;
    *(word *)&D0 = 1;                       // mov word ptr D0, 1
    eax = *(dword *)&g_memByte[523096];     // mov eax, lastPlayerPlayed
    A0 = eax;                               // mov A0, eax
    eax = *(dword *)&g_memByte[523087];     // mov eax, lastKeeperPlayed
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (flags.zero)
        goto cseg_7D45F;                    // jz short cseg_7D45F

    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 326524;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A0, offset goalie1Sprite
    if (flags.zero)
        goto cseg_7D455;                    // jz short cseg_7D455

    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 327756;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A0, offset goalie2Sprite
    if (!flags.zero)
        goto cseg_7D45F;                    // jnz short cseg_7D45F

cseg_7D455:;
    eax = *(dword *)&g_memByte[523087];     // mov eax, lastKeeperPlayed
    A0 = eax;                               // mov A0, eax

cseg_7D45F:;
    ax = *(word *)&g_memByte[336636];       // mov ax, statsTeam1Goals
    *(word *)&g_memByte[336624] = ax;       // mov statsTeam1GoalsCopy2, ax
    ax = *(word *)&g_memByte[336638];       // mov ax, statsTeam2Goals
    *(word *)&g_memByte[336626] = ax;       // mov statsTeam2GoalsCopy2, ax
    push(A6);                               // push A6
    SWOS::GoalScored();                     // call GoalScored
    pop(A6);                                // pop A6
    *(word *)&g_memByte[455982] = 1;        // mov goalCameraMode, 1
    *(word *)&g_memByte[456644] = 1;        // mov teamNumThatScored, 1

l_goal_handled:;
    eax = A6;                               // mov eax, A6
    *(dword *)&g_memByte[456648] = eax;     // mov teamScoredDataPtr, eax
    esi = A6;                               // mov esi, A6
    eax = readMemory(esi + 10, 4);          // mov eax, [esi+TeamGeneralInfo.inGameTeamPtr]
    *(dword *)&g_memByte[456652] = eax;     // mov teamScoredGamePtr, eax
    eax = readMemory(esi, 4);               // mov eax, [esi]
    A6 = eax;                               // mov A6, eax
    {
        int32_t dstSigned = A6;
        int32_t srcSigned = 522792;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A6, offset topTeamData
    if (flags.zero)
        goto cseg_7D4DD;                    // jz short cseg_7D4DD

    *(word *)&D3 = 0;                       // mov word ptr D3, 0
    *(byte *)&D4 = 199;                     // mov byte ptr D4, 0C7h
    goto l_play_goal_comment;               // jmp short @@play_goal_comment

cseg_7D4DD:;
    *(word *)&D3 = 4;                       // mov word ptr D3, 4
    *(byte *)&D4 = 124;                     // mov byte ptr D4, 7Ch

l_play_goal_comment:;
    {
        word src = *(word *)&g_memByte[456248];
        int16_t dstSigned = src;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp goalTypeScored, GT_OWN_GOAL
    if (flags.zero)
        goto l_own_goal;                    // jz short @@own_goal

    SWOS::PlayGoalComment();                // call PlayGoalComment
    goto l_check_if_home_goal;              // jmp short @@check_if_home_goal

l_own_goal:;
    SWOS::PlayOwnGoalComment();             // call PlayOwnGoalComment

l_check_if_home_goal:;
    {
        word src = *(word *)&g_memByte[456644];
        int16_t dstSigned = src;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp teamNumThatScored, 2
    if (flags.zero)
        goto l_guests_scored;               // jz short @@guests_scored

    SWOS::PlayHomeGoalSample();             // call PlayHomeGoalSample
    goto l_set_goal_state;                  // jmp short @@set_goal_state

l_guests_scored:;
    SWOS::PlayAwayGoalSample();             // call PlayAwayGoalSample

l_set_goal_state:;
    *(word *)&g_memByte[456262] = -1;       // mov stateGoal, -1
    *(word *)&g_memByte[523668] = 0;        // mov playRefereeWhistle, 0
    SWOS::Rand();                           // call Rand
    {
        word res = *(word *)&D0 >> 1;
        *(word *)&D0 = res;
    }                                       // shr word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 100;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 100
    ax = D0;                                // mov ax, word ptr D0
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = *(word *)&g_memByte[523148];       // mov ax, playingPenalties
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_penalty_scored;              // jnz @@penalty_scored

    ax = *(word *)&g_memByte[336636];       // mov ax, statsTeam1Goals
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = *(word *)&g_memByte[336638];       // mov ax, statsTeam2Goals
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    if (flags.zero)
        goto cseg_7D5F6;                    // jz cseg_7D5F6

    ax = *(word *)&g_memByte[336624];       // mov ax, statsTeam1GoalsCopy2
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = *(word *)&g_memByte[336626];       // mov ax, statsTeam2GoalsCopy2
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    if (flags.zero)
        goto cseg_7D5E0;                    // jz short cseg_7D5E0

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 1
    if (flags.zero)
        goto cseg_7D5A8;                    // jz short cseg_7D5A8

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = -1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, -1
    if (!flags.zero)
        goto cseg_7D5D5;                    // jnz short cseg_7D5D5

cseg_7D5A8:;
    ax = *(word *)&g_memByte[336636];       // mov ax, statsTeam1Goals
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = *(word *)&g_memByte[336638];       // mov ax, statsTeam2Goals
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 2
    if (flags.zero)
        goto cseg_7D5EB;                    // jz short cseg_7D5EB

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = -2;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, -2
    if (flags.zero)
        goto cseg_7D5EB;                    // jz short cseg_7D5EB

cseg_7D5D5:;
    *(word *)&D1 = 100;                     // mov word ptr D1, 100
    goto cseg_7D5FF;                        // jmp short cseg_7D5FF

cseg_7D5E0:;
    *(word *)&D1 = 300;                     // mov word ptr D1, 300
    goto cseg_7D5FF;                        // jmp short cseg_7D5FF

cseg_7D5EB:;
    *(word *)&D1 = 200;                     // mov word ptr D1, 200
    goto cseg_7D5FF;                        // jmp short cseg_7D5FF

cseg_7D5F6:;
    *(word *)&D1 = 200;                     // mov word ptr D1, 200

cseg_7D5FF:;
    SWOS::Rand();                           // call Rand
    ax = D0;                                // mov ax, word ptr D0
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int16_t>(res)) & (srcSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D1 = res;
    }                                       // add word ptr D1, ax

l_penalty_scored:;
    ax = D1;                                // mov ax, word ptr D1
    *(word *)&g_memByte[455978] = ax;       // mov goalCounter, ax
    *(word *)&g_memByte[521016] = 1;        // mov patternsGoalCounter, 1
    *(word *)&D1 = 336;                     // mov word ptr D1, 336
    *(word *)&D2 = 449;                     // mov word ptr D2, 449
    *(word *)&g_memByte[523118] = 0;        // mov gameState, ST_PLAYERS_TO_INITIAL_POSITIONS
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    goto l_break_handled;                   // jmp @@break_handled

l_not_in_goal_check_near_miss:;
    A6 = 328988;                            // mov A6, offset ballSprite
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    ax = (word)readMemory(esi + 40, 2);     // mov ax, word ptr [esi+(Sprite.z+2)]
    *(word *)&D3 = ax;                      // mov word ptr D3, ax
    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 2;
        word res = dstSigned + srcSigned;
        *(word *)&D3 = res;
    }                                       // add word ptr D3, 2
    {
        word src = (word)readMemory(esi + 44, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 768;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.speed], 768
    if (flags.carry)
        goto l_check_for_corner_goal_out;   // jb short @@check_for_corner_goal_out

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 290;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 290
    if (flags.sign != flags.overflow)
        goto l_check_for_corner_goal_out;   // jl short @@check_for_corner_goal_out

    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 381;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 381
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_check_for_corner_goal_out;   // jg short @@check_for_corner_goal_out

    {
        int16_t dstSigned = *(word *)&D3;
        int16_t srcSigned = 25;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D3, 25
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_check_for_corner_goal_out;   // jg short @@check_for_corner_goal_out

    push(D1);                               // push D1
    push(D2);                               // push D2
    push(D3);                               // push D3
    SWOS::PlayNearMissComment();            // call PlayNearMissComment
    pop(D3);                                // pop D3
    pop(D2);                                // pop D2
    pop(D1);                                // pop D1
    push(D1);                               // push D1
    push(D2);                               // push D2
    push(D3);                               // push D3
    SWOS::PlayMissGoalSample();             // call PlayMissGoalSample
    pop(D3);                                // pop D3
    pop(D2);                                // pop D2
    pop(D1);                                // pop D1
    *(word *)&g_memByte[523668] = 0;        // mov playRefereeWhistle, 0

l_check_for_corner_goal_out:;
    // fix original SWOS bug where penalty flag remains set when penalty is missed, but it's not near miss
    clearPenaltyFlag();
    A6 = 522792;                            // mov A6, offset topTeamData
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 129;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 129
    if (flags.sign == flags.overflow)
        goto l_not_upper_corner_goal_out;   // jge short @@not_upper_corner_goal_out

    eax = *(dword *)&g_memByte[523092];     // mov eax, lastTeamPlayed
    {
        int32_t dstSigned = A6;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A6, eax
    if (!flags.zero)
        goto l_upper_goal_out;              // jnz @@upper_goal_out

    A6 = 522940;                            // mov A6, offset bottomTeamData
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 336;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 336
    if (flags.sign != flags.overflow)
        goto l_left_upper_corner;           // jl @@left_upper_corner

    goto l_right_upper_corner;              // jmp @@right_upper_corner

l_not_upper_corner_goal_out:;
    A6 = 522940;                            // mov A6, offset bottomTeamData
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 769;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 769
    if (flags.zero || flags.sign != flags.overflow)
        goto l_ball_in_pitch;               // jle @@ball_in_pitch

    eax = *(dword *)&g_memByte[523092];     // mov eax, lastTeamPlayed
    {
        int32_t dstSigned = A6;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A6, eax
    if (!flags.zero)
        goto l_lower_goal_out;              // jnz @@lower_goal_out

    A6 = 522792;                            // mov A6, offset topTeamData
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 336;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 336
    if (flags.sign != flags.overflow)
        goto l_lower_left_corner;           // jl short @@lower_left_corner

    {
        word src = *(word *)&g_memByte[455960];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp forceLeftTeam, 1
    if (flags.zero)
        goto l_right_upper_corner;          // jz short @@right_upper_corner

    *(word *)&D1 = 585;                     // mov word ptr D1, 585
    *(word *)&D2 = 764;                     // mov word ptr D2, 764
    *(word *)&D3 = 6;                       // mov word ptr D3, 6
    *(byte *)&D4 = 193;                     // mov byte ptr D4, 0C1h
    *(word *)&g_memByte[523118] = 4;        // mov gameState, ST_CORNER_LEFT
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    goto l_its_a_corner;                    // jmp @@its_a_corner

l_lower_left_corner:;
    {
        word src = *(word *)&g_memByte[455960];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp forceLeftTeam, 1
    if (flags.zero)
        goto l_left_upper_corner;           // jz short @@left_upper_corner

    *(word *)&D1 = 86;                      // mov word ptr D1, 86
    *(word *)&D2 = 764;                     // mov word ptr D2, 764
    *(word *)&D3 = 2;                       // mov word ptr D3, 2
    *(byte *)&D4 = 7;                       // mov byte ptr D4, 7
    *(word *)&g_memByte[523118] = 5;        // mov gameState, ST_CORNER_RIGHT
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    goto l_its_a_corner;                    // jmp short @@its_a_corner

l_right_upper_corner:;
    *(word *)&D1 = 585;                     // mov word ptr D1, 585
    *(word *)&D2 = 134;                     // mov word ptr D2, 134
    *(word *)&D3 = 6;                       // mov word ptr D3, 6
    *(byte *)&D4 = 112;                     // mov byte ptr D4, 70h
    *(word *)&g_memByte[523118] = 5;        // mov gameState, ST_CORNER_RIGHT
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    goto l_its_a_corner;                    // jmp short @@its_a_corner

l_left_upper_corner:;
    *(word *)&D1 = 86;                      // mov word ptr D1, 86
    *(word *)&D2 = 134;                     // mov word ptr D2, 134
    *(word *)&D3 = 2;                       // mov word ptr D3, 2
    *(byte *)&D4 = 28;                      // mov byte ptr D4, 1Ch
    *(word *)&g_memByte[523118] = 4;        // mov gameState, ST_CORNER_LEFT
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1

l_its_a_corner:;
    esi = A6;                               // mov esi, A6
    eax = readMemory(esi + 14, 4);          // mov eax, [esi+TeamGeneralInfo.teamStatsPtr]
    A0 = eax;                               // mov A0, eax
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi + 2, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int16_t>(res)) & (srcSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 2, 2, src);
    }                                       // add [esi+TeamStatisticsData.cornersWon], 1
    enqueueCornerSample();                  // call EnqueueCornerSample
    goto l_break_handled;                   // jmp @@break_handled

l_upper_goal_out:;
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 336;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 336
    if (flags.sign != flags.overflow)
        goto l_left_upper_goal_out;         // jl short @@left_upper_goal_out

cseg_7D8C0:;
    *(word *)&D1 = 396;                     // mov word ptr D1, 396
    *(word *)&D2 = 154;                     // mov word ptr D2, 154
    *(word *)&D3 = 4;                       // mov word ptr D3, 4
    *(byte *)&D4 = 124;                     // mov byte ptr D4, 7Ch
    *(word *)&g_memByte[523118] = 1;        // mov gameState, ST_GOAL_OUT_LEFT
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    goto cseg_7D9C3;                        // jmp cseg_7D9C3

l_left_upper_goal_out:;
    *(word *)&D1 = 276;                     // mov word ptr D1, 276
    *(word *)&D2 = 154;                     // mov word ptr D2, 154
    *(word *)&D3 = 4;                       // mov word ptr D3, 4
    *(byte *)&D4 = 124;                     // mov byte ptr D4, 7Ch
    *(word *)&g_memByte[523118] = 2;        // mov gameState, ST_GOAL_OUT_RIGHT
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    goto cseg_7D9C3;                        // jmp cseg_7D9C3

l_lower_goal_out:;
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 336;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 336
    if (flags.sign != flags.overflow)
        goto l_left_lower_goal_out;         // jl short @@left_lower_goal_out

    {
        word src = *(word *)&g_memByte[455960];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp forceLeftTeam, 1
    if (flags.zero)
        goto cseg_7D8C0;                    // jz cseg_7D8C0

    *(word *)&D1 = 396;                     // mov word ptr D1, 396
    *(word *)&D2 = 744;                     // mov word ptr D2, 744
    *(word *)&D3 = 0;                       // mov word ptr D3, 0
    *(byte *)&D4 = 199;                     // mov byte ptr D4, 0C7h
    *(word *)&g_memByte[523118] = 2;        // mov gameState, ST_GOAL_OUT_RIGHT
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    goto cseg_7D9C3;                        // jmp short cseg_7D9C3

l_left_lower_goal_out:;
    {
        word src = *(word *)&g_memByte[455960];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp forceLeftTeam, 1
    if (flags.zero)
        goto l_left_upper_goal_out;         // jz @@left_upper_goal_out

    *(word *)&D1 = 276;                     // mov word ptr D1, 276
    *(word *)&D2 = 744;                     // mov word ptr D2, 744
    *(word *)&D3 = 0;                       // mov word ptr D3, 0
    *(byte *)&D4 = 199;                     // mov byte ptr D4, 0C7h
    *(word *)&g_memByte[523118] = 1;        // mov gameState, ST_GOAL_OUT_LEFT
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1

cseg_7D9C3:;
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 4, 2);      // mov ax, [esi+TeamGeneralInfo.playerNumber]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_its_goal_out;                // jnz short @@its_goal_out

    {
        byte res = *(byte *)&D4 & 187;
        *(byte *)&D4 = res;
        flags.carry = false;
        flags.overflow = false;
        flags.sign = (res & 0x80) != 0;
        flags.zero = res == 0;
    }                                       // and byte ptr D4, 0BBh

l_its_goal_out:;
    *(word *)&g_memByte[523102] = 1;        // mov goalOut, 1
    goto l_break_handled;                   // jmp @@break_handled

l_ball_in_pitch:;
    eax = *(dword *)&g_memByte[523092];     // mov eax, lastTeamPlayed
    A6 = eax;                               // mov A6, eax
    esi = A6;                               // mov esi, A6
    eax = readMemory(esi, 4);               // mov eax, [esi+TeamGeneralInfo.opponentsTeam]
    A6 = eax;                               // mov A6, eax
    {
        int16_t dstSigned = *(word *)&D1;
        int16_t srcSigned = 336;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D1, 336
    if (flags.sign != flags.overflow)
        goto l_left_half_of_pitch;          // jl short @@left_half_of_pitch

    *(word *)&D1 = 590;                     // mov word ptr D1, 590
    *(word *)&D3 = 6;                       // mov word ptr D3, 6
    *(byte *)&D4 = 241;                     // mov byte ptr D4, 0F1h
    {
        int32_t dstSigned = A6;
        int32_t srcSigned = 522792;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A6, offset topTeamData
    if (!flags.zero)
        goto l_rh_right_team_throw_in;      // jnz short @@rh_right_team_throw_in

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 342;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 342
    if (flags.sign != flags.overflow)
        goto cseg_7DAC7;                    // jl cseg_7DAC7

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 556;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 556
    if (flags.sign != flags.overflow)
        goto cseg_7DAD2;                    // jl cseg_7DAD2

    goto cseg_7DADD;                        // jmp cseg_7DADD

l_rh_right_team_throw_in:;
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 342;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 342
    if (flags.sign != flags.overflow)
        goto cseg_7DAFE;                    // jl cseg_7DAFE

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 556;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 556
    if (flags.sign != flags.overflow)
        goto cseg_7DAF3;                    // jl cseg_7DAF3

    goto cseg_7DAE8;                        // jmp short cseg_7DAE8

l_left_half_of_pitch:;
    *(word *)&D1 = 81;                      // mov word ptr D1, 81
    *(word *)&D3 = 2;                       // mov word ptr D3, 2
    *(byte *)&D4 = 31;                      // mov byte ptr D4, 1Fh
    {
        int32_t dstSigned = A6;
        int32_t srcSigned = 522792;
        dword res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = static_cast<uint32_t>(dstSigned) < static_cast<uint32_t>(srcSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
    }                                       // cmp A6, offset topTeamData
    if (!flags.zero)
        goto l_lh_right_team_throw_in;      // jnz short @@lh_right_team_throw_in

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 342;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 342
    if (flags.sign != flags.overflow)
        goto cseg_7DAE8;                    // jl short cseg_7DAE8

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 556;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 556
    if (flags.sign != flags.overflow)
        goto cseg_7DAF3;                    // jl short cseg_7DAF3

    goto cseg_7DAFE;                        // jmp short cseg_7DAFE

l_lh_right_team_throw_in:;
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 342;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 342
    if (flags.sign != flags.overflow)
        goto cseg_7DADD;                    // jl short cseg_7DADD

    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 556;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D2, 556
    if (flags.sign != flags.overflow)
        goto cseg_7DAD2;                    // jl short cseg_7DAD2

cseg_7DAC7:;
    *(word *)&g_memByte[523118] = 15;       // mov gameState, ST_THROW_IN_FORWARD_RIGHT
    goto l_enqueue_throw_in_sample;         // jmp short @@enqueue_throw_in_sample

cseg_7DAD2:;
    *(word *)&g_memByte[523118] = 16;       // mov gameState, ST_THROW_IN_CENTER_RIGHT
    goto l_enqueue_throw_in_sample;         // jmp short @@enqueue_throw_in_sample

cseg_7DADD:;
    *(word *)&g_memByte[523118] = 17;       // mov gameState, ST_THROW_IN_BACK_RIGHT
    goto l_enqueue_throw_in_sample;         // jmp short @@enqueue_throw_in_sample

cseg_7DAE8:;
    *(word *)&g_memByte[523118] = 18;       // mov gameState, ST_THROW_IN_FORWARD_LEFT
    goto l_enqueue_throw_in_sample;         // jmp short @@enqueue_throw_in_sample

cseg_7DAF3:;
    *(word *)&g_memByte[523118] = 19;       // mov gameState, ST_THROW_IN_CENTER_LEFT
    goto l_enqueue_throw_in_sample;         // jmp short @@enqueue_throw_in_sample

cseg_7DAFE:;
    *(word *)&g_memByte[523118] = 20;       // mov gameState, ST_THROW_IN_BACK_LEFT

l_enqueue_throw_in_sample:;
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    enqueueThrowInSample();                 // call EnqueueThrowInSample

l_break_handled:;
    {
        word src = *(word *)&g_memByte[455960];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp forceLeftTeam, 1
    if (!flags.zero)
        goto cseg_7DB29;                    // jnz short cseg_7DB29

    A6 = 522792;                            // mov A6, offset topTeamData

cseg_7DB29:;
    *(word *)&g_memByte[523116] = 101;      // mov gameStatePl, 101
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    ax = D1;                                // mov ax, word ptr D1
    *(word *)&g_memByte[523124] = ax;       // mov foulXCoordinate, ax
    ax = D2;                                // mov ax, word ptr D2
    *(word *)&g_memByte[523126] = ax;       // mov foulYCoordinate, ax
    ax = D3;                                // mov ax, word ptr D3
    *(word *)&g_memByte[523128] = ax;       // mov cameraDirection, ax
    al = D4;                                // mov al, byte ptr D4
    g_memByte[523130] = al;                 // mov byte ptr playerTurnFlags, al
    eax = A6;                               // mov eax, A6
    *(dword *)&g_memByte[523108] = eax;     // mov lastTeamPlayedBeforeBreak, eax
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    stopAllPlayers();                       // call StopAllPlayers
    *(word *)&g_memByte[449796] = 0;        // mov cameraXVelocity, 0
    *(word *)&g_memByte[449798] = 0;        // mov cameraYVelocity, 0

l_starting_the_game:;
    ax = *(word *)&g_memByte[523668];       // mov ax, playRefereeWhistle
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        return;                             // jz short @@out

    SWOS::PlayRefereeWhistleSample();       // call PlayRefereeWhistleSample
}

void resetBothTeamSpinTimers()
{
    swos.topTeamData.spinTimer = -1;
    swos.bottomTeamData.spinTimer = -1;
}

// Besides setting x and y coordinates, stops the ball and puts it to the ground (z = 0).
void setBallPosition(int x, int y)
{
    swos.ballSprite.speed = 0;
    swos.ballSprite.x = x;
    swos.ballSprite.y = y;
#ifdef SWOS_TEST
    swos.ballSprite.z.setWhole(0);
#else
    swos.ballSprite.z = 0;
#endif
    swos.ballSprite.destX = x;
    swos.ballSprite.destY = y;
    swos.ballSprite.deltaZ = 0;
}

// out:
//      A0 -> ball factor table, depending on game state
//
// Even indices are delta x, odd are delta y. They are added to ball
// delta x and y. Tables are different for each type of game halt.
// Called when player kicks the ball or does a pass.
//
void getBallDestCoordinatesTable()
{
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 15;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_THROW_IN_FORWARD_RIGHT
    if (flags.carry)
        goto l_not_throw_in;                // jb short @@not_throw_in

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 20;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_THROW_IN_BACK_LEFT
    if (!flags.carry && !flags.zero)
        goto l_not_throw_in;                // ja short @@not_throw_in

    {
        word src = *(word *)&g_memByte[523124];
        int16_t dstSigned = src;
        int16_t srcSigned = 336;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp foulXCoordinate, 336
    if (!flags.carry && !flags.zero)
        goto l_right_throw_in;              // ja short @@right_throw_in

    A0 = 523326;                            // mov A0, offset kLeftThrowInBallDestDelta
    return;                                 // retn

l_right_throw_in:;
    A0 = 523358;                            // mov A0, offset kRightThrowInBallDestDelta
    return;                                 // retn

l_not_throw_in:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 14;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PENALTY
    if (flags.zero)
        goto l_penalty;                     // jz short @@penalty

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 31;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PENALTIES
    if (!flags.zero)
        goto l_not_penalty;                 // jnz short @@not_penalty

l_penalty:;
    A0 = 523390;                            // mov A0, offset kPenaltyBallDestDelta
    return;                                 // retn

l_not_penalty:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 4;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_CORNER_LEFT
    if (flags.zero)
        goto l_corner;                      // jz short @@corner

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 5;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_CORNER_RIGHT
    if (!flags.zero)
        goto l_not_corner;                  // jnz short @@not_corner

l_corner:;
    {
        word src = *(word *)&g_memByte[523126];
        int16_t dstSigned = src;
        int16_t srcSigned = 449;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp foulYCoordinate, 449
    if (!flags.carry && !flags.zero)
        goto l_lower_corner;                // ja short @@lower_corner

    {
        word src = *(word *)&g_memByte[523124];
        int16_t dstSigned = src;
        int16_t srcSigned = 336;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp foulXCoordinate, 336
    if (!flags.carry && !flags.zero)
        goto l_upper_right_corner;          // ja short @@upper_right_corner

    A0 = 523422;                            // mov A0, offset kUpperLeftCornerBallDestDelta
    return;                                 // retn

l_upper_right_corner:;
    A0 = 523454;                            // mov A0, offset kUpperRightCornerBallDestDelta
    return;                                 // retn

l_lower_corner:;
    {
        word src = *(word *)&g_memByte[523124];
        int16_t dstSigned = src;
        int16_t srcSigned = 336;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp foulXCoordinate, 336
    if (!flags.carry && !flags.zero)
        goto l_lower_right_corner;          // ja short @@lower_right_corner

    A0 = 523486;                            // mov A0, offset kLowerLeftCornerBallDestDelta
    return;                                 // retn

l_lower_right_corner:;
    A0 = 523518;                            // mov A0, offset kLowerRightCornerBallDestDelta
    return;                                 // retn

l_not_corner:;
    A0 = 523294;                            // mov A0, offset kDefaultDestinations
}

static void calculateNextBallPosition()
{
    A0 = &swos.ballSprite;                  // mov A0, offset ballSprite
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi + 30, 4);          // mov eax, [esi+Sprite.x]
    D5 = eax;                               // mov D5, eax
    eax = readMemory(esi + 34, 4);          // mov eax, [esi+Sprite.y]
    D6 = eax;                               // mov D6, eax
    eax = readMemory(esi + 38, 4);          // mov eax, [esi+Sprite.z]
    D7 = eax;                               // mov D7, eax
    ax = (word)readMemory(esi + 44, 2);     // mov ax, [esi+Sprite.speed]
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_standing_still;              // jz @@standing_still

    eax = readMemory(esi + 46, 4);          // mov eax, [esi+Sprite.deltaX]
    D1 = eax;                               // mov D1, eax
    eax = readMemory(esi + 50, 4);          // mov eax, [esi+Sprite.deltaY]
    D2 = eax;                               // mov D2, eax
    eax = readMemory(esi + 54, 4);          // mov eax, [esi+Sprite.deltaZ]
    D3 = eax;                               // mov D3, eax
    eax = swos.kGravityConstant;            // mov eax, kGravityConstant
    D4 = eax;                               // mov D4, eax
    eax = D3;                               // mov eax, D3
    flags.carry = false;
    flags.overflow = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (!flags.zero && flags.sign == flags.overflow)
        goto l_ball_very_high;              // jg short @@ball_very_high

    ax = (word)readMemory(esi + 40, 2);     // mov ax, word ptr [esi+(Sprite.z+2)]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 20;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 20
    if (flags.carry || flags.zero)
        goto l_ball_low;                    // jbe @@ball_low

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 30;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 30
    if (flags.carry || flags.zero)
        goto l_ball_a_little_bit_high;      // jbe @@ball_a_little_bit_high

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 35;
        word res = dstSigned - srcSigned;
        flags.overflow = ((dstSigned ^ srcSigned) & (dstSigned ^ static_cast<int16_t>(res))) < 0;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 35
    if (flags.carry || flags.zero)
        goto l_ball_high;                   // jbe short @@ball_high

l_ball_very_high:;
    {
        dword res = D1 << 3;
        D1 = res;
    }                                       // shl D1, 3
    {
        dword res = D2 << 3;
        D2 = res;
    }                                       // shl D2, 3
    {
        dword res = D4 << 3;
        D4 = res;
    }                                       // shl D4, 3
    {
        int32_t res = *(int32_t *)&D7 >> 3;
        *(int32_t *)&D7 = res;
    }                                       // sar D7, 3

l_ball_still_in_the_air:;
    eax = D1;                               // mov eax, D1
    {
        int32_t dstSigned = D5;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        D5 = res;
    }                                       // add D5, eax
    eax = D2;                               // mov eax, D2
    {
        int32_t dstSigned = D6;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        D6 = res;
    }                                       // add D6, eax
    eax = D4;                               // mov eax, D4
    {
        int32_t dstSigned = D3;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        D3 = res;
    }                                       // sub D3, eax
    eax = D3;                               // mov eax, D3
    {
        int32_t dstSigned = D7;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int32_t>(res)) & (srcSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
        D7 = res;
    }                                       // add D7, eax
    if (!flags.sign)
        goto l_ball_still_in_the_air;       // jns short @@ball_still_in_the_air

    goto l_standing_still;                  // jmp @@standing_still

l_ball_high:;
    {
        dword res = D1 << 2;
        D1 = res;
    }                                       // shl D1, 2
    {
        dword res = D2 << 2;
        D2 = res;
    }                                       // shl D2, 2
    {
        dword res = D4 << 2;
        D4 = res;
    }                                       // shl D4, 2
    {
        int32_t res = *(int32_t *)&D7 >> 2;
        *(int32_t *)&D7 = res;
    }                                       // sar D7, 2

l_ball_still_in_the_air_2:;
    eax = D1;                               // mov eax, D1
    {
        int32_t dstSigned = D5;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        D5 = res;
    }                                       // add D5, eax
    eax = D2;                               // mov eax, D2
    {
        int32_t dstSigned = D6;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        D6 = res;
    }                                       // add D6, eax
    eax = D4;                               // mov eax, D4
    {
        int32_t dstSigned = D3;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        D3 = res;
    }                                       // sub D3, eax
    eax = D3;                               // mov eax, D3
    {
        int32_t dstSigned = D7;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int32_t>(res)) & (srcSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
        D7 = res;
    }                                       // add D7, eax
    if (!flags.sign)
        goto l_ball_still_in_the_air_2;     // jns short @@ball_still_in_the_air_2

    goto l_standing_still;                  // jmp short @@standing_still

l_ball_a_little_bit_high:;
    {
        dword res = D1 << 1;
        D1 = res;
    }                                       // shl D1, 1
    {
        dword res = D2 << 1;
        D2 = res;
    }                                       // shl D2, 1
    {
        dword res = D4 << 1;
        D4 = res;
    }                                       // shl D4, 1
    {
        int32_t res = *(int32_t *)&D7 >> 1;
        *(int32_t *)&D7 = res;
    }                                       // sar D7, 1

l_ball_still_in_the_air_3:;
    eax = D1;                               // mov eax, D1
    {
        int32_t dstSigned = D5;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        D5 = res;
    }                                       // add D5, eax
    eax = D2;                               // mov eax, D2
    {
        int32_t dstSigned = D6;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        D6 = res;
    }                                       // add D6, eax
    eax = D4;                               // mov eax, D4
    {
        int32_t dstSigned = D3;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        D3 = res;
    }                                       // sub D3, eax
    eax = D3;                               // mov eax, D3
    {
        int32_t dstSigned = D7;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int32_t>(res)) & (srcSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
        D7 = res;
    }                                       // add D7, eax
    if (!flags.sign)
        goto l_ball_still_in_the_air_3;     // jns short @@ball_still_in_the_air_3

    goto l_standing_still;                  // jmp short @@standing_still

l_ball_low:;
    eax = D1;                               // mov eax, D1
    {
        int32_t dstSigned = D5;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        D5 = res;
    }                                       // add D5, eax
    eax = D2;                               // mov eax, D2
    {
        int32_t dstSigned = D6;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        D6 = res;
    }                                       // add D6, eax
    eax = D4;                               // mov eax, D4
    {
        int32_t dstSigned = D3;
        int32_t srcSigned = eax;
        dword res = dstSigned - srcSigned;
        D3 = res;
    }                                       // sub D3, eax
    eax = D3;                               // mov eax, D3
    {
        int32_t dstSigned = D7;
        int32_t srcSigned = eax;
        dword res = dstSigned + srcSigned;
        flags.overflow = ((dstSigned ^ static_cast<int32_t>(res)) & (srcSigned ^ static_cast<int32_t>(res))) < 0;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        flags.sign = (res & 0x80000000) != 0;
        flags.zero = res == 0;
        D7 = res;
    }                                       // add D7, eax
    if (!flags.sign)
        goto l_ball_low;                    // jns short @@ball_low

l_standing_still:;
    ax = D5;                                // mov ax, word ptr D5
    {
        word tmp = *(word *)((byte *)&D5 + 2);
        *(word *)((byte *)&D5 + 2) = ax;
        ax = tmp;
    }                                       // xchg ax, word ptr D5+2
    *(word *)&D5 = ax;                      // mov word ptr D5, ax
    ax = D6;                                // mov ax, word ptr D6
    {
        word tmp = *(word *)((byte *)&D6 + 2);
        *(word *)((byte *)&D6 + 2) = ax;
        ax = tmp;
    }                                       // xchg ax, word ptr D6+2
    *(word *)&D6 = ax;                      // mov word ptr D6, ax
    ax = D5;                                // mov ax, word ptr D5
    *(word *)&g_memByte[523660] = ax;       // mov ballNextX, ax
    ax = D6;                                // mov ax, word ptr D6
    *(word *)&g_memByte[523662] = ax;       // mov ballNextY, ax
}

// in:
//      D5 -  ball x
//      A0 -> ball sprite
//
// Reverse dest x direction around x (by the same delta x amount).
//
static void reverseDestXDirection()
{
    eax = D5;                               // mov eax, D5
    D4 = eax;                               // mov D4, eax
    {
        word tmp = *(word *)((byte *)&D4 + 2);
        *(word *)((byte *)&D4 + 2) = ax;
        ax = tmp;
    }                                       // xchg ax, word ptr D4+2
    *(word *)&D4 = ax;                      // mov word ptr D4, ax
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 58, 2);     // mov ax, [esi+Sprite.destX]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = D4;                                // mov ax, word ptr D4
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    ax = D0;                                // mov ax, word ptr D0
    {
        int16_t dstSigned = *(word *)&D4;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D4 = res;
    }                                       // sub word ptr D4, ax
    ax = D4;                                // mov ax, word ptr D4
    writeMemory(esi + 58, 2, ax);           // mov [esi+Sprite.destX], ax
}

// =============== S U B R O U T I N E =======================================

// in:
//      D6 -  ball y
//      A0 -> ball sprite
//
// Reverse dest y direction around y (by the same delta y amount).
//
static void reverseDestYDirection()
{
    eax = D6;                               // mov eax, D6
    D4 = eax;                               // mov D4, eax
    {
        word tmp = *(word *)((byte *)&D4 + 2);
        *(word *)((byte *)&D4 + 2) = ax;
        ax = tmp;
    }                                       // xchg ax, word ptr D4+2
    *(word *)&D4 = ax;                      // mov word ptr D4, ax
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi + 60, 2);     // mov ax, [esi+Sprite.destY]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = D4;                                // mov ax, word ptr D4
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    ax = D0;                                // mov ax, word ptr D0
    {
        int16_t dstSigned = *(word *)&D4;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D4 = res;
    }                                       // sub word ptr D4, ax
    ax = D4;                                // mov ax, word ptr D4
    writeMemory(esi + 60, 2, ax);           // mov [esi+Sprite.destY], ax
}
