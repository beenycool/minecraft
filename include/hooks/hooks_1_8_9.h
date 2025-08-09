#pragma once

#include <jni.h>
#include <cstdint>

namespace Hooks_1_8_9 {
    // Hook target definitions
    struct Target {
        enum Opcode : uint8_t {
            INVOKEVIRTUAL = 0,
            INVOKESPECIAL = 1,
            INVOKESTATIC = 2,
            PUTFIELD = 3
        };

        Opcode opcode;
        uint8_t occurrence;
        const char* className;
        const char* methodName;
        const char* methodSig;
        const char* mcpClassName;
        const char* mcpMethodName;
        const char* mcpMethodSig;
        const char* hookClassName;
        const char* hookMethodName;
        const char* hookMethodSig;
    };

    // JNI native method definitions
    struct JNINativeMethodEntry {
        const char* name;
        const char* signature;
        void* fnPtr;
    };

    // Hook targets for 1.8.9
    static constexpr const Target VANILLA_1_8_9[19] = {
        {Target::Opcode::INVOKEVIRTUAL, 1, "bdb", "b", "(IIZ)V", "bcz", "b", "(II)V", "SlinkyHooks9", "unloadChunk", "(Ljava/lang/Object;II)V"},
        {Target::Opcode::INVOKESPECIAL, 1, "bdb", "b", "(Lcj;Lalz;)Z", "adm", "a", "(Lcj;Lalz;I)Z", "SlinkyHooks9", "setBlock_1_8", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;I)Z"},
        {Target::Opcode::INVOKEVIRTUAL, 1, "ave", "s", "()V", "bfk", "a", "(F)V", "SlinkyHooks9", "getMouseOver", "(Ljava/lang/Object;F)V"},
        {Target::Opcode::INVOKESPECIAL, 1, "ave", "s", "()V", "ave", "aw", "()V", "SlinkyHooks9", "clickMouse", "(Ljava/lang/Object;)V"},
        {Target::Opcode::INVOKESPECIAL, 2, "ave", "s", "()V", "ave", "ax", "()V", "SlinkyHooks9", "rightClickMouse", "(Ljava/lang/Object;)V"},
        {Target::Opcode::INVOKEVIRTUAL, 1, "ave", "s", "()V", "bdb", "i", "()V", "SlinkyHooks9", "updateEntities", "(Ljava/lang/Object;)V"},
        {Target::Opcode::INVOKEVIRTUAL, 1, "ave", "av", "()V", "bfw", "a", "(Z)V", "SlinkyHooks9", "bindFrameBuffer", "(Ljava/lang/Object;Z)V"},
        {Target::Opcode::INVOKESTATIC, 1, "ave", "h", "()V", "org/lwjgl/opengl/Display", "update", "()V", "SlinkyHooks9", "updateDisplay", "()V"},
        {Target::Opcode::INVOKEVIRTUAL, 3, "pr", "g", "(FF)V", "pr", "a", "(FFF)V", "SlinkyHooks9", "moveFlying", "(Ljava/lang/Object;FFF)V"},
        {Target::Opcode::INVOKESPECIAL, 1, "bln", "a", "(Lbet;DDDFF)V", "bjl", "a", "(Lpr;DDDFF)V", "SlinkyHooks9", "renderPlayer", "(Ljava/lang/Object;Ljava/lang/Object;DDDFF)V"},
        {Target::Opcode::INVOKEVIRTUAL, 1, "bjl", "b", "(Lpr;DDD)V", "bjl", "a", "(Lpr;)Z", "SlinkyHooks9", "canRenderName", "(Ljava/lang/Object;Ljava/lang/Object;)Z"},
        {Target::Opcode::INVOKESTATIC, 2, "bfk", "a", "(IFJ)V", "bfl", "m", "(I)V", "SlinkyHooks9", "postWorldClear_1_8", "(I)V"},
        {Target::Opcode::INVOKESTATIC, 1, "bfk", "a", "(IFJ)V", "auz", "a", "(Lwn;Z)V", "SlinkyHooks9", "updateActiveRenderInfo", "(Ljava/lang/Object;Z)V"},
        {Target::Opcode::INVOKESPECIAL, 1, "bfk", "a", "(FI)V", "bfk", "f", "(F)V", "SlinkyHooks9", "orientCamera", "(Ljava/lang/Object;F)V"},
        {Target::Opcode::INVOKEVIRTUAL, 1, "bcy", "a", "(Lgp;)V", "bdb", "b", "(IIIIII)V", "SlinkyHooks9", "invalidateChunk", "(Ljava/lang/Object;IIIIII)V"},
        {Target::Opcode::INVOKEVIRTUAL, 1, "bcy", "a", "(Lgo;)V", "bdb", "b", "(IIIIII)V", "SlinkyHooks9", "invalidateChunk", "(Ljava/lang/Object;IIIIII)V"},
        {Target::Opcode::INVOKEVIRTUAL, 1, "pk", "d", "(DDD)V", "pk", "av", "()Z", "SlinkyHooks9", "shouldClipLedge", "(Ljava/lang/Object;)Z"},
        {Target::Opcode::PUTFIELD, 4, "biu", "a", "(Ladm;Lavn;Lpk;Lpk;Lavh;F)V", "biu", "e", "F", "SlinkyHooks9", "setRenderY", "(Ljava/lang/Object;F)V"},
        {Target::Opcode::PUTFIELD, 2, "biu", "a", "(Ladm;Lavn;Lpk;Lpk;Lavh;F)V", "biu", "f", "F", "SlinkyHooks9", "setRenderX", "(Ljava/lang/Object;F)V"}
    };

        // JNI native methods
        static JNINativeMethodEntry METHODS[20] = {
        {"bindFrameBuffer", "(Ljava/lang/Object;Z)V", (void*)(std::uintptr_t)(0x3fc0)},
        {"getMouseOver", "(Ljava/lang/Object;F)V", (void*)(std::uintptr_t)(0xb2c0)},
        {"setRenderX", "(Ljava/lang/Object;F)V", (void*)(std::uintptr_t)(0x3390)},
        {"updateEntities", "(Ljava/lang/Object;)V", (void*)(std::uintptr_t)(0xb780)},
        {"canRenderName", "(Ljava/lang/Object;Ljava/lang/Object;)Z", (void*)(std::uintptr_t)(0x32f0)},
        {"setBlock_1_8", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;I)Z", (void*)(std::uintptr_t)(0x2fb0)},
        {"renderPlayer", "(Ljava/lang/Object;Ljava/lang/Object;DDDFF)V", (void*)(std::uintptr_t)(0x3250)},
        {"orientCamera", "(Ljava/lang/Object;F)V", (void*)(std::uintptr_t)(0x3500)},
        {"updateDisplay", "()V", (void*)(std::uintptr_t)(0xad00)},
        {"setBlock_1_7", "(Ljava/lang/Object;IIILjava/lang/Object;II)Z", (void*)(std::uintptr_t)(0x30a0)},
        {"clickMouse", "(Ljava/lang/Object;)V", (void*)(std::uintptr_t)(0x3740)},
        {"moveFlying", "(Ljava/lang/Object;FFF)V", (void*)(std::uintptr_t)(0x2ba0)},
        {"shouldClipLedge", "(Ljava/lang/Object;)Z", (void*)(std::uintptr_t)(0x2e40)},
        {"setRenderY", "(Ljava/lang/Object;F)V", (void*)(std::uintptr_t)(0x3320)},
        {"rightClickMouse", "(Ljava/lang/Object;)V", (void*)(std::uintptr_t)(0x35e0)},
        {"invalidateChunk", "(Ljava/lang/Object;IIIIII)V", (void*)(std::uintptr_t)(0x2ed0)},
        {"unloadChunk", "(Ljava/lang/Object;II)V", (void*)(std::uintptr_t)(0x3190)},
        {"postWorldClear_1_8", "(I)V", (void*)(std::uintptr_t)(0x3ec0)},
        {"postWorldClear_1_7", "(I)V", (void*)(std::uintptr_t)(0x3ed0)},
        {"updateActiveRenderInfo", "(Ljava/lang/Object;Z)V", (void*)(std::uintptr_t)(0x3400)}
    };

    // Initialize hooks for 1.8.9
    bool initialize(JNIEnv* env);
}