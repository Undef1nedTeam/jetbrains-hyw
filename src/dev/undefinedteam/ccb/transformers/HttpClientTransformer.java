package dev.undefinedteam.ccb.transformers;

import dev.undefinedteam.ccb.hooks.URLFilter;
import dev.undefinedteam.obfuscator.annotations.natives.NativeDefine;
import dev.undefinedteam.asm.ClassReader;
import dev.undefinedteam.asm.ClassWriter;
import dev.undefinedteam.asm.Type;
import dev.undefinedteam.asm.tree.ClassNode;
import dev.undefinedteam.asm.tree.FieldInsnNode;
import dev.undefinedteam.asm.tree.InsnList;
import dev.undefinedteam.asm.tree.InsnNode;
import dev.undefinedteam.asm.tree.MethodInsnNode;
import dev.undefinedteam.asm.tree.MethodNode;
import dev.undefinedteam.asm.tree.VarInsnNode;

import static dev.undefinedteam.asm.Opcodes.*;

@NativeDefine
public class HttpClientTransformer {
    public static byte[] transform(String className, byte[] classBytes) {
        ClassReader reader = new ClassReader(classBytes);
        ClassNode node = new ClassNode();

        reader.accept(node, 0);
        for (MethodNode mn : node.methods) {
            if (!"openServer".equals(mn.name) || !"()V".equals(mn.desc)) continue;
            InsnList list = new InsnList();
            list.add(new VarInsnNode(25, 0));
            list.add(new FieldInsnNode(
                    180,
                    "sun/net/www/http/HttpClient",
                    "url",
                    "Ljava/net/URL;"
            ));
            list.add(new MethodInsnNode(
                    184,
                    Type.getInternalName(URLFilter.class),
                    "testURL",
                    "(Ljava/net/URL;)Ljava/net/URL;",
                    false
            ));
            list.add(new InsnNode(87));
            mn.instructions.insert(list);
        }
        ClassWriter writer = new ClassWriter(3);
        node.accept(writer);
        return writer.toByteArray();
    }
}
