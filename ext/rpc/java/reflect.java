/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license.html.                                     |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Sam Ruby (rubys@us.ibm.com)                                  |
   +----------------------------------------------------------------------+
 */

package net.php;

import java.lang.reflect.*;
import java.util.*;
import java.beans.*;

public class reflect {

  static { loadLibrary("reflect"); }
  
  protected static void loadLibrary(String property) {
    try {
      ResourceBundle bundle = ResourceBundle.getBundle("net.php."+property);
      System.loadLibrary(bundle.getString("library"));
    } catch (Exception e) {
      e.printStackTrace();
    }
  }

  //
  // Native methods
  //
  private static native void setResultFromString(long result, byte value[]);
  private static native void setResultFromLong(long result, long value);
  private static native void setResultFromDouble(long result, double value);
  private static native void setResultFromBoolean(long result, boolean value);
  private static native void setResultFromObject(long result, Object value);
  private static native void setResultFromArray(long result);
  private static native long nextElement(long array);
  private static native void setException(long result, String value);
  public  static native void setEnv();

  //
  // Helper routines which encapsulate the native methods
  //
  public static void setResult(long result, Object value) {
    if (value == null) return;

    if (value instanceof java.lang.String) {

      setResultFromString(result, ((String)value).getBytes());

    } else if (value instanceof java.lang.Number) {

       if (value instanceof java.lang.Integer ||
           value instanceof java.lang.Short ||
           value instanceof java.lang.Byte) {
         setResultFromLong(result, ((Number)value).longValue());
       } else {
         /* Float, Double, BigDecimal, BigInteger, Double, Long, ... */
         setResultFromDouble(result, ((Number)value).doubleValue());
       }

    } else if (value instanceof java.lang.Boolean) {

      setResultFromBoolean(result, ((Boolean)value).booleanValue());

    } else if (value.getClass().isArray()) {

      long length = Array.getLength(value);
      setResultFromArray(result);
      for (int i=0; i<length; i++) {
        setResult(nextElement(result), Array.get(value, i));
      }

    } else {

      setResultFromObject(result, value);

    }
  }

  static void setException(long result, Throwable e) {
    if (e instanceof InvocationTargetException) {
      Throwable t = ((InvocationTargetException)e).getTargetException();
      if (t!=null) e=t;
    }

    setException(result, e.toString());
  }

  //
  // Create an new instance of a given class
  //
  public static void CreateObject(String name, Object args[], long result) {
    try {
      Vector matches = new Vector();

      Constructor cons[] = Class.forName(name).getConstructors();
      for (int i=0; i<cons.length; i++) {
        if (cons[i].getParameterTypes().length == args.length) {
          matches.addElement(cons[i]);
        }
      }

      Constructor selected = (Constructor)select(matches, args);

      if (selected == null) {
        if (args.length > 0) {
          throw new InstantiationException("No matching constructor found");
        } else {
          // for classes which have no visible constructor, return the class
          // useful for classes like java.lang.System and java.util.Calendar.
          setResult(result, Class.forName(name));
          return;
        }
      }

      Object coercedArgs[] = coerce(selected.getParameterTypes(), args);
      setResult(result, selected.newInstance(coercedArgs));

    } catch (Exception e) {
      setException(result, e);
    }
  }

  //
  // Select the best match from a list of methods
  //
  private static Object select(Vector methods, Object args[]) {
    if (methods.size() == 1) return methods.firstElement();

    Object selected = null;
    int best = Integer.MAX_VALUE;

    for (Enumeration e = methods.elements(); e.hasMoreElements(); ) {
      Object element = e.nextElement();
      int weight=0;

      Class parms[] = (element instanceof Method) ?
        ((Method)element).getParameterTypes() : 
        ((Constructor)element).getParameterTypes();

      for (int i=0; i<parms.length; i++) {
        if (parms[i].isInstance(args[i])) {
	  for (Class c=parms[i]; (c=c.getSuperclass()) != null; ) {
            if (!c.isInstance(args[i])) break;
            weight++;
          }
        } else if (parms[i].isInstance("")) {
	  if (!(args[i] instanceof byte[]))
	    weight+=9999;
        } else if (parms[i].isPrimitive()) {
          Class c=parms[i];
	  if (args[i] instanceof Number) {
            if (c==Boolean.TYPE) weight+=5;
            if (c==Character.TYPE) weight+=4;
            if (c==Byte.TYPE) weight+=3;
            if (c==Short.TYPE) weight+=2;
            if (c==Integer.TYPE) weight++;
            if (c==Float.TYPE) weight++;
          } else if (args[i] instanceof Boolean) {
            if (c!=Boolean.TYPE) weight+=9999;
          } else if (args[i] instanceof String) {
            if (c== Character.TYPE || ((String)args[i]).length()>0)
              weight+=((String)args[i]).length();
            else
              weight+=9999;
          } else {
	    weight+=9999;
          }
        } else {
	  weight+=9999;
        }
      } 

      if (weight < best) {
        if (weight == 0) return element;
        best = weight;
        selected = element;
      }
    }

    return selected;
  }

  //
  // Coerce arguments when possible to conform to the argument list.
  // Java's reflection will automatically do widening conversions,
  // unfortunately PHP only supports wide formats, so to be practical
  // some (possibly lossy) conversions are required.
  //
  private static Object[] coerce(Class parms[], Object args[]) {
    Object result[] = args;
    for (int i=0; i<args.length; i++) {
      if (args[i] instanceof byte[] && !parms[i].isArray()) {
        result[i] = new String((byte[])args[i]);
      } else if (args[i] instanceof Number && parms[i].isPrimitive()) {
        if (result==args) result=(Object[])result.clone();
        Class c = parms[i];
        Number n = (Number)args[i];
        if (c == Boolean.TYPE) result[i]=new Boolean(0.0!=n.floatValue());
        if (c == Byte.TYPE)    result[i]=new Byte(n.byteValue());
        if (c == Short.TYPE)   result[i]=new Short(n.shortValue());
        if (c == Integer.TYPE) result[i]=new Integer(n.intValue());
        if (c == Float.TYPE)   result[i]=new Float(n.floatValue());
        if (c == Long.TYPE && !(n instanceof Long)) 
          result[i]=new Long(n.longValue());
      }
    }
    return result;
  }

  //
  // Invoke a method on a given object
  //
  public static void Invoke
    (Object object, String method, Object args[], long result)
  {

    try {
      Vector matches = new Vector();

      // gather
      for (Class jclass = object.getClass();;jclass=(Class)object) {
        while (!Modifier.isPublic(jclass.getModifiers())) {
          // OK, some joker gave us an instance of a non-public class
          // This often occurs in the case of enumerators
          // Substitute the first public interface in its place,
          // and barring that, try the superclass
          Class interfaces[] = jclass.getInterfaces();
          jclass=jclass.getSuperclass();
          for (int i=interfaces.length; i-->0;) {
            if (Modifier.isPublic(interfaces[i].getModifiers())) {
              jclass=interfaces[i];
            }
          }
        }
        Method methods[] = jclass.getMethods();
        for (int i=0; i<methods.length; i++) {
          if (methods[i].getName().equalsIgnoreCase(method) &&
              methods[i].getParameterTypes().length == args.length) {
            matches.addElement(methods[i]);
          }
        }

        // try a second time with the object itself, if it is of type Class
        if (!(object instanceof Class) || (jclass==object)) break;
      }

      Method selected = (Method)select(matches, args);
      if (selected == null) throw new NoSuchMethodException(method);

      Object coercedArgs[] = coerce(selected.getParameterTypes(), args);
      setResult(result, selected.invoke(object, coercedArgs));

    } catch (Exception e) {
      setException(result, e);
    }
  }

  //
  // Get or Set a property
  //
  public static void GetSetProp
    (Object object, String prop, Object args[], long result)
  {
    try {

      for (Class jclass = object.getClass();;jclass=(Class)object) {
        while (!Modifier.isPublic(jclass.getModifiers())) {
          // OK, some joker gave us an instance of a non-public class
          // Substitute the first public interface in its place,
          // and barring that, try the superclass
          Class interfaces[] = jclass.getInterfaces();
          jclass=jclass.getSuperclass();
          for (int i=interfaces.length; i-->0;) {
            if (Modifier.isPublic(interfaces[i].getModifiers())) {
              jclass=interfaces[i];
            }
          }
        }
        BeanInfo beanInfo = Introspector.getBeanInfo(jclass);
        PropertyDescriptor props[] = beanInfo.getPropertyDescriptors();
        for (int i=0; i<props.length; i++) {
          if (props[i].getName().equalsIgnoreCase(prop)) {
            Method method;
            if (args!=null && args.length>0) {
              method=props[i].getWriteMethod();
            } else {
              method=props[i].getReadMethod();
            }
            setResult(result, method.invoke(object, args));
            return;
          }
        }

        Field jfields[] = jclass.getFields();
        for (int i=0; i<jfields.length; i++) {
          if (jfields[i].getName().equalsIgnoreCase(prop)) {
            if (args!=null && args.length>0) {
              jfields[i].set(object, args[0]);
            } else {
              setResult(result, jfields[i].get(object));
            }
            return;
          }
        }

        // try a second time with the object itself, if it is of type Class
        if (!(object instanceof Class) || (jclass==object)) break;
      }

    } catch (Exception e) {
      setException(result, e);
    }
  }

  //
  // Helper routines for the C implementation
  //
  public static Object MakeArg(boolean b) { return new Boolean(b); }
  public static Object MakeArg(long l)    { return new Long(l); }
  public static Object MakeArg(double d)  { return new Double(d); }
}
