def lgen(name,lam,visibility=["//visibility:public"]):
    native.genrule(name = name + "/gen", srcs = [lam,"//lgen:lgen"], outs = ["lg.cc"], cmd = "cat $(location " + lam + ") | ./$(location //lgen:lgen) > $(OUTS)")
    native.cc_binary(name = name, srcs = [":" + name + "/gen"], visibility = visibility)