package dev.undefinedteam.ccb.transformers;

import dev.undefinedteam.ccb.hooks.ArgsFilter;
import dev.undefinedteam.ccb.hooks.ResultFilter;
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
public class BigIntegerTransformer {
    public static byte[] transform(String className, byte[] classBytes) {
        ClassReader reader = new ClassReader(classBytes);
        ClassNode node = new ClassNode();
        reader.accept(node, 0);


        for (MethodNode mn : node.methods) {
            if (!"oddModPow".equals(mn.name) || !"(Ljava/math/BigInteger;Ljava/math/BigInteger;)Ljava/math/BigInteger;".equals(mn.desc)) continue;

            InsnList argList = new InsnList();
            argList.add(new VarInsnNode(25, 0));
            argList.add(new VarInsnNode(25, 1));
            argList.add(new VarInsnNode(25, 2));
            argList.add(new MethodInsnNode(
                    184,
                    Type.getInternalName(ArgsFilter.class),
                    "testFilter",
                    "(Ljava/math/BigInteger;Ljava/math/BigInteger;Ljava/math/BigInteger;)[Ljava/math/BigInteger;",
                    false
            ));
            argList.add(new VarInsnNode(58, 3));
            argList.add(new InsnNode(1));
            argList.add(new VarInsnNode(25, 3));
            LabelNode argLabel = new LabelNode();
            argList.add(new JumpInsnNode(165, argLabel));
            argList.add(new VarInsnNode(25, 3));
            argList.add(new InsnNode(3));
            argList.add(new InsnNode(50));
            argList.add(new VarInsnNode(58, 1));
            argList.add(new VarInsnNode(25, 3));
            argList.add(new InsnNode(4));
            argList.add(new InsnNode(50));
            argList.add(new VarInsnNode(58, 2));
            argList.add(argLabel);
            mn.instructions.insert(argList);

            InsnList resultList = new InsnList();
            resultList.add(new VarInsnNode(25, 0));
            resultList.add(new VarInsnNode(25, 1));
            resultList.add(new VarInsnNode(25, 2));
            resultList.add(new MethodInsnNode(
                    184,
                    Type.getInternalName(ResultFilter.class),
                    "testFilter",
                    "(Ljava/math/BigInteger;Ljava/math/BigInteger;Ljava/math/BigInteger;)Ljava/math/BigInteger;",
                    false
            ));
            resultList.add(new VarInsnNode(58, 3));
            resultList.add(new InsnNode(1));
            resultList.add(new VarInsnNode(25, 3));
            LabelNode resultLabel = new LabelNode();
            resultList.add(new JumpInsnNode(165, resultLabel));
            resultList.add(new VarInsnNode(25, 3));
            resultList.add(new InsnNode(176));
            resultList.add(resultLabel);
            mn.instructions.insert(resultList);
        }
        ClassWriter writer = new ClassWriter(3);
        node.accept(writer);
        return writer.toByteArray();
    }
}
