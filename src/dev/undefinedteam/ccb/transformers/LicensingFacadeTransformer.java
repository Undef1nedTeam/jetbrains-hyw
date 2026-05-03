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

import static dev.undefinedteam.asm.Opcodes.*;

@NativeDefine
public class LicensingFacadeTransformer {
    public static byte[] transform(String className, byte[] classBytes) {
        ClassReader reader = new ClassReader(classBytes);
        ClassNode node = new ClassNode();

        reader.accept(node, 0);
        for (MethodNode m : node.methods) {
            if (!"getLicenseExpirationDate".equals(m.name)) continue;
            InsnList list = new InsnList();

            list.add(new MethodInsnNode(
                    INVOKESTATIC,
                    Type.getInternalName(StackTraceRule.class),
                    "hook",
                    "()Ljava/util/Date;",
                    false
            ));
            list.add(new InsnNode(DUP));

            LabelNode ret = new LabelNode();
            list.add(new JumpInsnNode(IFNULL, ret));
            list.add(new InsnNode(ARETURN));
            list.add(ret);
            list.add(new InsnNode(POP));

            m.instructions.insert(list);
        }
        ClassWriter writer = new ClassWriter(3);
        node.accept(writer);
        return writer.toByteArray();
    }
}
