package dev.undefinedteam.ccb.transformers;

import dev.undefinedteam.ccb.hooks.StackTraceRule;
import dev.undefinedteam.obfuscator.annotations.natives.NativeDefine;
import dev.undefinedteam.asm.ClassReader;
import dev.undefinedteam.asm.ClassWriter;
import dev.undefinedteam.asm.Type;
import dev.undefinedteam.asm.tree.ClassNode;
import dev.undefinedteam.asm.tree.InsnList;
import dev.undefinedteam.asm.tree.InsnNode;
import dev.undefinedteam.asm.tree.JumpInsnNode;
import dev.undefinedteam.asm.tree.LabelNode;
import dev.undefinedteam.asm.tree.MethodInsnNode;
import dev.undefinedteam.asm.tree.MethodNode;
import dev.undefinedteam.asm.tree.TypeInsnNode;

import static dev.undefinedteam.asm.Opcodes.*;

@NativeDefine
// 这个是啥啊。。

public class PluginManagerCoreTransformer {
    public static byte[] transform(String className, byte[] classBytes) {
        return classBytes;

//        ClassReader reader = new ClassReader(classBytes);
//        ClassNode node = new ClassNode();
//        reader.accept(node, 0);
//
//        for (MethodNode m : node.methods) {
//            LabelNode labelNode;
//            InsnList list;
//            if ("isPluginInstalled".equals(m.name)) {
//                list = new InsnList();
//                list.add(new MethodInsnNode(
//                        INVOKESTATIC,
//                        Type.getInternalName(StackTraceRule.class),
//                        "check",
//                        "()Z",
//                        false
//                ));
//                labelNode = new LabelNode();
//                list.add(new JumpInsnNode(IFEQ, labelNode));
//                list.add(new InsnNode(ICONST_0));
//                list.add(new InsnNode(IRETURN));
//                list.add(labelNode);
//                m.instructions.insert(list);
//                continue;
//            }
//            if (!"getPlugins".equals(m.name)) continue;
//            list = new InsnList();
//            list.add(new MethodInsnNode(
//                    INVOKESTATIC,
//                    Type.getInternalName(StackTraceRule.class),
//                    "check",
//                    "()Z",
//                    false
//            ));
//            labelNode = new LabelNode();
//            list.add(new JumpInsnNode(IFEQ, labelNode));
//            list.add(new InsnNode(ICONST_0));
//            list.add(new TypeInsnNode(ANEWARRAY, "java/lang/Object"));
//            list.add(new InsnNode(ARETURN));
//            list.add(labelNode);
//            m.instructions.insert(list);
//        }
//        ClassWriter writer = new ClassWriter(3);
//        node.accept(writer);
//        return writer.toByteArray();
    }
}
