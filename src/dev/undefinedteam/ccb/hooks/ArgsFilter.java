package dev.undefinedteam.ccb.hooks;

import dev.undefinedteam.obfuscator.annotations.natives.NativeDefine;

import java.math.BigInteger;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

@NativeDefine
public class ArgsFilter {
    private static final Set<String> l1cached;
    private static final Map<String, BigInteger[]> l2cached;

    static {
        List<String> rules = List.of(
                "65537,24773058818499217187577663886010908531303294206336895556072197892590450942803807164562754911175164262596715237551312004078542654996496301487027034803410086499747369353221485073240039340641397198525027728751956658900801359887190562885573922317930300068615009483578963467556425525328780085523172495307229112069939166202511721671904748968934606589702999279663332403655662225374084460291376706916679151764149324177444374590606643838366605181996272409014933080082205048098737253668016260658830645459388519595314928290853199112791333551144805347785109465401055719331231478162870216035573012645710763533896540021550083104281->3,24773058818499217187577663886010908531303294206336895556072197892590450942803807164562754911175164262596715237551312004078542654996496301487027034803410086499747369353221485073240039340641397198525027728751956658900801359887190562885573922317930300068615009483578963467556425525328780085523172495307229112069939166202511721671904748968934606589702999279663332403655662225374084460291376706916679151764149324177444374590606643838366605181996272409014933080082205048098737253668016260658830645459388519595314928290853199112791333551144805347785109465401055719331231478162870216035573012645710763533896540021550083104281"
        );

        l1cached = new HashSet<>();
        l2cached = new HashMap<>();
        for (String rule : rules) {
            String[] sections = rule.split("->", 2);
            if (2 != sections.length) {
                continue;
            }
            if (-1 == sections[1].indexOf(44)) {
                continue;
            }
            l1cached.add(
                    Arrays.stream(sections[0].split(","))
                            .map(s -> String.valueOf(new BigInteger((String) s).intValue()))
                            .collect(Collectors.joining(","))
            );
            String[] tmp = sections[1].split(",");
            l2cached.put(sections[0], new BigInteger[]{new BigInteger(tmp[0]), new BigInteger(tmp[1])});
        }
    }

    public static BigInteger[] testFilter(BigInteger x, BigInteger y, BigInteger z) {
        if (l1cached.contains(y.intValue() + "," + z.intValue())) {
            return l2cached.getOrDefault(y + "," + z, null);
        }
        return null;
    }
}
