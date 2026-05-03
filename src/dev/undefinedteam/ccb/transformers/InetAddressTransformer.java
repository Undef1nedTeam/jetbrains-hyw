package dev.undefinedteam.ccb.transformers;

import dev.undefinedteam.ccb.hooks.DNSFilter;
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
import dev.undefinedteam.asm.tree.VarInsnNode;

import static dev.undefinedteam.asm.Opcodes.*;

@NativeDefine
public class InetAddressTransformer {
    public static byte[] transform(String className, byte[] classBytes) {
        ClassReader reader = new ClassReader(classBytes);
        ClassNode node = new ClassNode(327680);
        reader.accept(node, 0);
        for (MethodNode m : node.methods) {
            InsnList list;
            if ("getAllByName".equals(m.name) && "(Ljava/lang/String;Ljava/net/InetAddress;)[Ljava/net/InetAddress;".equals(m.desc)) {
                list = new InsnList();
                list.add(new VarInsnNode(ALOAD, 0));
                list.add(new MethodInsnNode(
                        INVOKESTATIC,
                        Type.getInternalName(DNSFilter.class),
                        "testQuery",
                        "(Ljava/lang/String;)Ljava/lang/String;",
                        false
                ));
                list.add(new InsnNode(POP));
                m.instructions.insert(list);
                continue;
            }
            if (!"isReachable".equals(m.name) || !"(Ljava/net/NetworkInterface;II)Z".equals(m.desc)) continue;
            list = new InsnList();
            list.add(new VarInsnNode(ALOAD, 0));
            list.add(new MethodInsnNode(
                    INVOKESTATIC,
                    Type.getInternalName(DNSFilter.class),
                    "testReachable",
                    "(Ljava/net/InetAddress;)Ljava/lang/Object;",
                    false
            ));
            list.add(new VarInsnNode(ASTORE, 4));
            list.add(new InsnNode(ACONST_NULL));
            list.add(new VarInsnNode(ALOAD, 4));

            LabelNode label1 = new LabelNode();
            list.add(new JumpInsnNode(IF_ACMPEQ, label1));
            list.add(new InsnNode(ICONST_0));
            list.add(new InsnNode(IRETURN));
            list.add(label1);
            m.instructions.insert(list);
        }
        ClassWriter writer = new ClassWriter(ClassWriter.COMPUTE_FRAMES | ClassWriter.COMPUTE_MAXS);
        node.accept(writer);
        return writer.toByteArray();
    }
}
