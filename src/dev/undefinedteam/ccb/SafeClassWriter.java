package dev.undefinedteam.ccb;

import java.io.IOException;
import java.io.InputStream;
import jdk.internal.org.objectweb.asm.ClassReader;
import jdk.internal.org.objectweb.asm.ClassWriter;

public class SafeClassWriter extends ClassWriter {
   private final ClassLoader loader;

   public SafeClassWriter(ClassReader cr, ClassLoader loader, int flags) {
      super(cr, flags);
      this.loader = loader != null ? loader : ClassLoader.getSystemClassLoader();
   }

   protected String getCommonSuperClass(String type1, String type2) {
      try {
         ClassReader info1 = this.typeInfo(type1);
         ClassReader info2 = this.typeInfo(type2);
         if ((info1.getAccess() & 512) != 0) {
            return this.typeImplements(type2, info2, type1) ? type1 : "java/lang/Object";
         }

         if ((info2.getAccess() & 512) != 0) {
            return this.typeImplements(type1, info1, type2) ? type2 : "java/lang/Object";
         }

         StringBuilder b1 = this.typeAncestors(type1, info1);
         StringBuilder b2 = this.typeAncestors(type2, info2);
         String result = "java/lang/Object";
         int end1 = b1.length();
         int end2 = b2.length();

         while (true) {
            int start1 = b1.lastIndexOf(";", end1 - 1);
            int start2 = b2.lastIndexOf(";", end2 - 1);
            if (start1 == -1 || start2 == -1 || end1 - start1 != end2 - start2) {
               return result;
            }

            String p1 = b1.substring(start1 + 1, end1);
            String p2 = b2.substring(start2 + 1, end2);
            if (!p1.equals(p2)) {
               return result;
            }

            result = p1;
            end1 = start1;
            end2 = start2;
         }
      } catch (IOException e) {
         throw new RuntimeException(e.toString());
      }
   }

   private StringBuilder typeAncestors(String type, ClassReader info) throws IOException {
      StringBuilder b = new StringBuilder();

      while (!"java/lang/Object".equals(type)) {
         b.append(';').append(type);
         type = info.getSuperName();
         info = this.typeInfo(type);
      }

      return b;
   }

   private boolean typeImplements(String type, ClassReader info, String itf) throws IOException {
      while (!"java/lang/Object".equals(type)) {
         String[] itfs = info.getInterfaces();

         for (String s : itfs) {
            if (s.equals(itf)) {
               return true;
            }
         }

         for (String s : itfs) {
            if (this.typeImplements(s, this.typeInfo(s), itf)) {
               return true;
            }
         }

         type = info.getSuperName();
         info = this.typeInfo(type);
      }

      return false;
   }

   private ClassReader typeInfo(String type) throws IOException {
      String resource = type + ".class";
      InputStream is = this.loader.getResourceAsStream(resource);
      if (is == null) {
         throw new IOException("Cannot create ClassReader for type " + type);
      }

      try {
         return new ClassReader(is);
      } finally {
         is.close();
      }
   }
}
