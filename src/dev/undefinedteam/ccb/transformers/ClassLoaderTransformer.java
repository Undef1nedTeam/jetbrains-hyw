package dev.undefinedteam.ccb.transformers;

import dev.undefinedteam.ccb.hooks.ClassFilter;
import dev.undefinedteam.obfuscator.annotations.natives.NativeDefine;
import dev.undefinedteam.asm.ClassReader;
import dev.undefinedteam.asm.ClassWriter;
import dev.undefinedteam.asm.Type;
import dev.undefinedteam.asm.tree.*;

import static dev.undefinedteam.asm.Opcodes.*;

@NativeDefine
public class ClassLoaderTransformer {
    public static byte[] transform(String name, byte[] bytes) {
        ClassReader reader = new ClassReader(bytes);
        ClassNode node = new ClassNode();
        reader.accept(node, 0);

        if (name.equals("com/intellij/ide/plugins/cl/PluginClassLoader")) {
            for (MethodNode m : node.methods) {
                if (!"loadClass".equals(m.name)) continue;

                InsnList list = new InsnList();
                list.add(new VarInsnNode(ALOAD, 1));
                list.add(new MethodInsnNode(
                        INVOKESTATIC,
                        Type.getInternalName(ClassFilter.class),
                        "check",
                        "(Ljava/lang/String;)V",
                        false
                ));
                m.instructions.insert(list);
            }
        } else if (name.equals("java/lang/Class")) {
            for (MethodNode mn : node.methods) {
                if (!"forName".equals(mn.name) || !mn.desc.startsWith("(Ljava/lang/String;")) continue;
                InsnList list = new InsnList();
                list.add(new VarInsnNode(ALOAD, 0));
                list.add(new MethodInsnNode(
                        INVOKESTATIC,
                        Type.getInternalName(ClassFilter.class),
                        "check",
                        "(Ljava/lang/String;)V",
                        false
                ));
                mn.instructions.insert(list);
            }
        }
        ClassWriter writer = new ClassWriter(3);
        node.accept(writer);
        return writer.toByteArray();
    }
}
