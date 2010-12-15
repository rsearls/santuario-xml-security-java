
/*
 * Copyright  1999-2004 The Apache Software Foundation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
package org.apache.xml.security.transforms;



import java.io.IOException;

import javax.xml.parsers.ParserConfigurationException;

import org.apache.xml.security.c14n.CanonicalizationException;
import org.apache.xml.security.c14n.InvalidCanonicalizerException;
import org.apache.xml.security.signature.XMLSignatureInput;
import org.xml.sax.SAXException;


/**
 * Base class which all Transform algorithms extend. The common methods that
 * have to be overridden are the {@link #enginePerformTransform} method.
 *
 * @author Christian Geuer-Pollmann
 */
public abstract class TransformSpi {

   /** {@link org.apache.commons.logging} logging facility */
    static org.apache.commons.logging.Log log = 
        org.apache.commons.logging.LogFactory.getLog(TransformSpi.class.getName());

   protected Transform _transformObject = null;
   protected void setTransform(Transform transform) {
      this._transformObject = transform;
   }
   /**
    * Tests whether Transform implemenation class is need Octect Stream as input of Transformation
    *
    * @return true if need Octect Stream as input
    */
   public abstract boolean wantsOctetStream();

   /**
    * Tests whether Transform implemenation class is need Octect Stream as input of Transformation
    *
    * @return true if need Node Set as input
    */
   public abstract boolean wantsNodeSet();

   /**
    * Tests whether Transform implemenation class will generate Octect Stream as output of result of Transformation
    *
    * @return true if will returns Octect Stream as output
    */
   public abstract boolean returnsOctetStream();

  /**
    * Tests whether Transform implemenation class will generate Node Set as output of result of Transformation
    *
    * @return true if will returns node set as output
    */
   public abstract boolean returnsNodeSet();

   /**
    * The mega method which MUST be implemented by the Transformation Algorithm.
    *
    * @param input {@link XMLSignatureInput} as the input of transformation
    * @return {@link XMLSignatureInput} as the result of transformation
    * @throws CanonicalizationException
    * @throws IOException
    * @throws InvalidCanonicalizerException
    * @throws NotYetImplementedException
    * @throws ParserConfigurationException
    * @throws SAXException
    * @throws TransformationException
    */
   protected abstract XMLSignatureInput enginePerformTransform(
      XMLSignatureInput input)
         throws IOException,
                CanonicalizationException, InvalidCanonicalizerException,
                TransformationException, ParserConfigurationException,
                SAXException;

   /**
    * Returns the URI representation of <code>Transformation algorithm</code>
    *
    * @return the URI representation of <code>Transformation algorithm</code>
    */
   protected abstract String engineGetURI();
}
