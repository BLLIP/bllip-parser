This directory contains the beginnings of a higher level Java interface to
BLLIP Parser with the ultimate hope of being distributed on Maven Central
and/or other Java package repositories. For now, it is **unsupported** and
subject to reorganization, changing APIs, etc. If you are a Java packaging
expert, please feel free to help out! Please do not file issues against
the higher level Java interface until it stabilizes.

See `issue #46 <https://github.com/BLLIP/bllip-parser/issues/46>`_ for
the current status. The ``main()`` function in ``BllipParser`` includes some
basic example code.

Semi-quick and dirty compilation instructions: (**unsupported!**)

    1. You'll need some JVM, SWIG, Maven, and standard GNU compilation
       tools installed.
    2. Edit the ``SWIG_JAVA_GCCFLAGS``, ``SWIG_LINKER_FLAGS``, and
       ``SWIG_JAVA_CLASSPATH`` flags in the main ``Makefile``.  All of these
       may be specified as environment variables instead of editing
       the ``Makefile``.
    3. run ``make swig-java`` in the top-level directory. This eventually
       call ``mvn install`` which should install a jar file to your maven
       repository. Now you should be able to include ``bllipparser`` with
       something like this in your ``pom.xml``::

        <dependency>
          <groupId>edu.brown.cs.bllip</groupId>
          <artifactId>bllipparser</artifactId>
          <version>1.0.0</version>
        </dependency>
