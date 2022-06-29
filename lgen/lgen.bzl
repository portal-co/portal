def lgen(name,lams,visibility=["//visibility:public"]):
    native.genrule(name = name + "/gen", srcs = lams,tools = ["//lgen:lgen"], outs = ["lg.cc"], cmd = "cat $(SRCS) | tr -d '\n' | ./$(location //lgen:lgen) > $(OUTS)")
    native.cc_binary(name = name, srcs = [":" + name + "/gen"], visibility = visibility)

