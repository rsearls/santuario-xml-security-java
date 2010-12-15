/*
 * Copyright 2002-2004 The Apache Software Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * imitations under the License.
 */

/*
 * XSEC
 *
 * xtest := basic test application to run through a series of tests of
 *			the XSEC library.
 *
 * Author(s): Berin Lautenbach
 *
 * $Id$
 *
 */

#include <xsec/framework/XSECDefs.hpp> 

#include <cassert>

#include <memory.h>
#include <iostream>
#include <stdlib.h>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/XMLFormatter.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLException.hpp>
#include <xercesc/util/Janitor.hpp>

#include <xsec/transformers/TXFMOutputFile.hpp>
#include <xsec/dsig/DSIGTransformXPath.hpp>
#include <xsec/dsig/DSIGTransformXPathFilter.hpp>
#include <xsec/dsig/DSIGTransformC14n.hpp>
#include <xsec/dsig/DSIGObject.hpp>

// XALAN

#ifndef XSEC_NO_XALAN

#include <xalanc/XPath/XPathEvaluator.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>

XALAN_USING_XALAN(XPathEvaluator)
XALAN_USING_XALAN(XalanTransformer)

#endif

// XSEC

#include <xsec/utils/XSECPlatformUtils.hpp>
#include <xsec/framework/XSECProvider.hpp>
#include <xsec/canon/XSECC14n20010315.hpp>
#include <xsec/dsig/DSIGReference.hpp>
#include <xsec/framework/XSECError.hpp>
#include <xsec/dsig/DSIGSignature.hpp>
#include <xsec/utils/XSECNameSpaceExpander.hpp>
#include <xsec/utils/XSECDOMUtils.hpp>
#include <xsec/utils/XSECBinTXFMInputStream.hpp>
#include <xsec/enc/XSECCryptoException.hpp>
#include <xsec/dsig/DSIGKeyInfoX509.hpp>
#include <xsec/dsig/DSIGKeyInfoName.hpp>
#include <xsec/dsig/DSIGKeyInfoPGPData.hpp>
#include <xsec/dsig/DSIGKeyInfoSPKIData.hpp>
#include <xsec/dsig/DSIGKeyInfoMgmtData.hpp>
#include <xsec/xenc/XENCCipher.hpp>
#include <xsec/xenc/XENCEncryptedData.hpp>
#include <xsec/xenc/XENCEncryptedKey.hpp>
#include <xsec/xenc/XENCEncryptionMethod.hpp>

#include <xsec/enc/XSECCryptoSymmetricKey.hpp>

#if defined (HAVE_OPENSSL)
#	include <xsec/enc/OpenSSL/OpenSSLCryptoKeyHMAC.hpp>
#	include <xsec/enc/OpenSSL/OpenSSLCryptoKeyRSA.hpp>
#	include <openssl/rand.h>
#	include <openssl/evp.h>
#	include <openssl/pem.h>
#endif
#if defined (HAVE_WINCAPI)
#	include <xsec/enc/WinCAPI/WinCAPICryptoKeyHMAC.hpp>
#	include <xsec/enc/WinCAPI/WinCAPICryptoKeyRSA.hpp>
#	include <xsec/enc/WinCAPI/WinCAPICryptoProvider.hpp>
#endif

using std::ostream;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;


/*
 * Because of all the characters, it's easiest to inject entire Xerces namespace
 * into global
 */

XERCES_CPP_NAMESPACE_USE

// --------------------------------------------------------------------------------
//           Global variables
// --------------------------------------------------------------------------------

bool	g_printDocs = false;
bool	g_useWinCAPI = false;
bool    g_haveAES = true;

// --------------------------------------------------------------------------------
//           Known "Good" Values
// --------------------------------------------------------------------------------

unsigned char createdDocRefs [9][20] = {
	{ 0x51, 0x3c, 0xb5, 0xdf, 0xb9, 0x1e, 0x9d, 0xaf, 0xd4, 0x4a,
	  0x95, 0x79, 0xf1, 0xd6, 0x54, 0xe, 0xb0, 0xb0, 0x29, 0xe3, },
	{ 0x51, 0x3c, 0xb5, 0xdf, 0xb9, 0x1e, 0x9d, 0xaf, 0xd4, 0x4a, 
	  0x95, 0x79, 0xf1, 0xd6, 0x54, 0xe, 0xb0, 0xb0, 0x29, 0xe3, },
	{ 0x52, 0x74, 0xc3, 0xe4, 0xc5, 0xf7, 0x20, 0xb0, 0xd9, 0x52, 
	  0xdb, 0xb3, 0xee, 0x46, 0x66, 0x8f, 0xe1, 0xb6, 0x30, 0x9d, },
	{ 0x5a, 0x14, 0x9c, 0x5a, 0x40, 0x34, 0x51, 0x4f, 0xef, 0x1d, 
	  0x85, 0x44, 0xc7, 0x2a, 0xd3, 0xd2, 0x2, 0xed, 0x67, 0xb4, },
	{ 0x88, 0xd1, 0x65, 0xed, 0x2a, 0xe7, 0xc0, 0xbd, 0xea, 0x3e, 
	  0xe6, 0xf3, 0xd4, 0x8c, 0xf7, 0xdd, 0xc8, 0x85, 0xa9, 0x6d, },
	{ 0x52, 0x74, 0xc3, 0xe4, 0xc5, 0xf7, 0x20, 0xb0, 0xd9, 0x52, 
	  0xdb, 0xb3, 0xee, 0x46, 0x66, 0x8f, 0xe1, 0xb6, 0x30, 0x9d, },
	{ 0x52, 0x74, 0xc3, 0xe4, 0xc5, 0xf7, 0x20, 0xb0, 0xd9, 0x52, 
	  0xdb, 0xb3, 0xee, 0x46, 0x66, 0x8f, 0xe1, 0xb6, 0x30, 0x9d, },
	{ 0x3c, 0x80, 0x4, 0x94, 0xa5, 0xbe, 0xf6, 0x16, 0x40, 0xe0, 
  	  0x24, 0xd5, 0x65, 0x39, 0xc, 0x18, 0x21, 0x3d, 0xa5, 0x51, },
  	{ 0x51, 0x3c, 0xb5, 0xdf, 0xb9, 0x1e, 0x9d, 0xaf, 0xd4, 0x4a, 
	  0x95, 0x79, 0xf1, 0xd6, 0x54, 0xe, 0xb0, 0xb0, 0x29, 0xe3, }

};

// --------------------------------------------------------------------------------
//           Some test data
// --------------------------------------------------------------------------------

// "CN=<Test,>,O=XSEC  "

XMLCh s_tstDName[] = {

	chLatin_C,
	chLatin_N,
	chEqual,
	chOpenAngle,
	chLatin_T,
	chLatin_e,
	chLatin_s,
	chLatin_t,
	chComma,
	chCloseAngle,
	chComma,
	chLatin_O,
	chEqual,
	chLatin_X,
	chLatin_S,
	chLatin_E,
	chLatin_C,
	chSpace,
	chSpace,
	chNull

};

XMLCh s_tstKeyName[] = {

	chLatin_F, chLatin_r, chLatin_e, chLatin_d, chSingleQuote,
	chLatin_s, chSpace, chLatin_n, chLatin_a, chLatin_m,
	chLatin_e, chNull
};

XMLCh s_tstPGPKeyID[] = {

	chLatin_D, chLatin_u, chLatin_m, chLatin_m, chLatin_y, chSpace,
	chLatin_P, chLatin_G, chLatin_P, chSpace,
	chLatin_I, chLatin_D, chNull
};

XMLCh s_tstPGPKeyPacket[] = {

	chLatin_D, chLatin_u, chLatin_m, chLatin_m, chLatin_y, chSpace,
	chLatin_P, chLatin_G, chLatin_P, chSpace,
	chLatin_P, chLatin_a, chLatin_c, chLatin_k, chLatin_e, chLatin_t, chNull
};

XMLCh s_tstSexp1[] = {

	chLatin_D, chLatin_u, chLatin_m, chLatin_m, chLatin_y, chSpace,
	chLatin_S, chLatin_e, chLatin_x, chLatin_p, chDigit_1, chNull
};

XMLCh s_tstSexp2[] = {

	chLatin_D, chLatin_u, chLatin_m, chLatin_m, chLatin_y, chSpace,
	chLatin_S, chLatin_e, chLatin_x, chLatin_p, chDigit_2, chNull
};

XMLCh s_tstMgmtData[] = {

	chLatin_D, chLatin_u, chLatin_m, chLatin_m, chLatin_y, chSpace,
	chLatin_M, chLatin_g, chLatin_m, chLatin_t, chSpace,
	chLatin_D, chLatin_a, chLatin_t, chLatin_a, chNull

};

XMLCh s_tstCarriedKeyName[] = {

	chLatin_D, chLatin_u, chLatin_m, chLatin_m, chLatin_y, chSpace,
	chLatin_C, chLatin_a, chLatin_r, chLatin_r, chLatin_y, chNull

};

XMLCh s_tstRecipient[] = {

	chLatin_D, chLatin_u, chLatin_m, chLatin_m, chLatin_y, chSpace,
	chLatin_R, chLatin_e, chLatin_c, chLatin_i, chLatin_p,
	chLatin_i, chLatin_e, chLatin_n, chLatin_t, chNull

};

XMLCh s_tstEncoding[] = {
	chLatin_B, chLatin_a, chLatin_s, chLatin_e, chDigit_6, chDigit_4, chNull
};

XMLCh s_tstMimeType[] = {
	chLatin_i, chLatin_m, chLatin_a, chLatin_g, chLatin_e,
	chForwardSlash, chLatin_p, chLatin_n, chLatin_g, chNull
};

unsigned char s_tstOAEPparams[] = "12345678";

unsigned char s_tstBase64EncodedString[] = "YmNkZWZnaGlqa2xtbm9wcRrPXjQ1hvhDFT+EdesMAPE4F6vlT+y0HPXe0+nAGLQ8";
char s_tstDecryptedString[] = "A test encrypted secret";

// --------------------------------------------------------------------------------
//           Some test keys
// --------------------------------------------------------------------------------

// A PKCS8 PEM encoded PrivateKey structure (not Encrypted)

char s_tstRSAPrivateKey[] = "\n\
-----BEGIN RSA PRIVATE KEY-----\n\
MIICXAIBAAKBgQDQj3pktZckAzwshRnfvLhz3daNU6xpAzoHo3qjCftxDwH1RynP\n\
A5eycJVkV8mwH2C1PFktpjtQTZ2CvPjuKmUV5zEvmYzuIo6SWYaVZN/PJjzsEZMa\n\
VA+U8GhfX1YF/rsuFzXCi8r6FVd3LN//pXHEwoDGdJUdlpdVEuX1iFKlNQIDAQAB\n\
AoGAYQ7Uc7e6Xa0PvNw4XVHzOSC870pISxqQT+u5b9R+anAEhkQW5dsTJpyUOX1N\n\
RCRmGhG6oq7gnY9xRN1yr0uVfJNtc9/HnzJL7L1jeJC8Ub+zbEBvNuPDL2P21ArW\n\
tcXRycUlfRCRBLop7rfOYPXsjtboAGnQY/6hK4rOF4XGrQUCQQD3Euj+0mZqRRZ4\n\
M1yN2wVP0mKOMg2i/HZXaNeVd9X/wyBgK6b7BxHf6onf/mIBWnJnRBlvdCrSdhuT\n\
lPKEoSgvAkEA2BhfWwQihqD4qJcV65nfosjzOZG41rHX69nIqHI7Ejx5ZgeQByH9\n\
Ym96yXoSpZj9ZlFsJYNogTBBnUBjs+jL2wJAFjpVS9eR7y2X/+hfA0QZDj1XMIPA\n\
RlGANAzymDfXwNLFLuG+fAb+zK5FCSnRl12TvUabIzPIRnbptDVKPDRjcQJBALn8\n\
0CVv+59P8HR6BR3QRBDBT8Xey+3NB4Aw42lHV9wsPHg6ThY1hPYx6MZ70IzCjmZ/\n\
8cqfvVRjijWj86wm0z0CQFKfRfBRraOZqfmOiAB4+ILhbJwKBBO6avX9TPgMYkyN\n\
mWKCxS+9fPiy1iI+G+B9xkw2gJ9i8P81t7fsOvdTDFA=\n\
-----END RSA PRIVATE KEY-----";

static char s_keyStr[] = "abcdefghijklmnopqrstuvwxyzabcdef";


// --------------------------------------------------------------------------------
//           Find a node
// --------------------------------------------------------------------------------

DOMNode * findNode(DOMNode * n, XMLCh * name) {

	if (XMLString::compareString(name, n->getNodeName()) == 0)
		return n;

	DOMNode * c = n->getFirstChild();

	while (c != NULL) {

		if (c->getNodeType() == DOMNode::ELEMENT_NODE) {

			DOMNode * s = findNode(c, name);
			if (s != NULL)
				return s;

		}

		c = c->getNextSibling();

	}

	return NULL;

}

// --------------------------------------------------------------------------------
//           Create a key
// --------------------------------------------------------------------------------

XSECCryptoKeyHMAC * createHMACKey(const unsigned char * str) {

	// Create the HMAC key
	XSECCryptoKeyHMAC * hmacKey;

#if defined (HAVE_OPENSSL) && defined(HAVE_WINCAPI)

	if (g_useWinCAPI == true) {
		hmacKey = new WinCAPICryptoKeyHMAC(0);
	}
	else {
		hmacKey = new OpenSSLCryptoKeyHMAC();
	}
#else
#	if defined (HAVE_OPENSSL)
		hmacKey = new OpenSSLCryptoKeyHMAC();
#	else
#		if defined (HAVE_WINCAPI)
		hmacKey = new WinCAPICryptoKeyHMAC(0);
#		endif
#	endif
#endif

	hmacKey->setKey((unsigned char *) str, (unsigned int) strlen((char *)str));

	return hmacKey;

}

// --------------------------------------------------------------------------------
//           Utility function for outputting hex data
// --------------------------------------------------------------------------------

void outputHex(unsigned char * buf, int len) {

	cout << std::hex;
	for (int i = 0; i < len; ++i) {
		cout << "0x" << (unsigned int) buf[i] << ", ";
	}
	cout << std::ios::dec << endl;

}

// --------------------------------------------------------------------------------
//           Create a basic document
// --------------------------------------------------------------------------------

DOMDocument * createTestDoc(DOMImplementation * impl) {

	DOMDocument *doc = impl->createDocument(
				0,                    // root element namespace URI.
				MAKE_UNICODE_STRING("ADoc"),            // root element name
				NULL);// DOMDocumentType());  // document type object (DTD).

	DOMElement *rootElem = doc->getDocumentElement();
	rootElem->setAttributeNS(DSIGConstants::s_unicodeStrURIXMLNS, 
		MAKE_UNICODE_STRING("xmlns:foo"), MAKE_UNICODE_STRING("http://www.foo.org"));

	DOMElement  * prodElem = doc->createElement(MAKE_UNICODE_STRING("product"));
	rootElem->appendChild(prodElem);

	DOMText    * prodDataVal = doc->createTextNode(MAKE_UNICODE_STRING("XMLSecurityC"));
	prodElem->appendChild(prodDataVal);

	DOMElement  *catElem = doc->createElement(MAKE_UNICODE_STRING("category"));
	rootElem->appendChild(catElem);
	catElem->setAttribute(MAKE_UNICODE_STRING("idea"), MAKE_UNICODE_STRING("great"));

	DOMText    *catDataVal = doc->createTextNode(MAKE_UNICODE_STRING("XML Security Tools"));
	catElem->appendChild(catDataVal);

	return doc;

}
// --------------------------------------------------------------------------------
//           Output a document if so required
// --------------------------------------------------------------------------------

void outputDoc(DOMImplementation * impl, DOMDocument * doc) {

	if (g_printDocs == false)
		return;

	DOMWriter         *theSerializer = ((DOMImplementationLS*)impl)->createDOMWriter();

	theSerializer->setEncoding(MAKE_UNICODE_STRING("UTF-8"));
	if (theSerializer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, false))
		theSerializer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, false);


	XMLFormatTarget *formatTarget = new StdOutFormatTarget();

	cerr << endl;

	theSerializer->writeNode(formatTarget, *doc);
	
	cout << endl;

	cerr << endl;

	delete theSerializer;
	delete formatTarget;

}

// --------------------------------------------------------------------------------
//           Unit test helper functions
// --------------------------------------------------------------------------------

bool reValidateSig(DOMImplementation *impl, DOMDocument * inDoc, XSECCryptoKey *k) {

	// Take a signature in DOM, serialise and re-validate

	try {

		DOMWriter *theSerializer = ((DOMImplementationLS*)impl)->createDOMWriter();

		theSerializer->setEncoding(MAKE_UNICODE_STRING("UTF-8"));

		if (theSerializer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, false))
			theSerializer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, false);

		MemBufFormatTarget *formatTarget = new MemBufFormatTarget();

		theSerializer->writeNode(formatTarget, *inDoc);

		// Copy to a new buffer
		int len = formatTarget->getLen();
		char * mbuf = new char [len + 1];
		memcpy(mbuf, formatTarget->getRawBuffer(), len);
		mbuf[len] = '\0';

		delete theSerializer;
		delete formatTarget;

		/*
		 * Re-parse
		 */

		XercesDOMParser parser;
		
		parser.setDoNamespaces(true);
		parser.setCreateEntityReferenceNodes(true);

		MemBufInputSource* memIS = new MemBufInputSource ((const XMLByte*) mbuf, 
																len, "XSECMem");

		parser.parse(*memIS);
		DOMDocument * doc = parser.adoptDocument();


		delete(memIS);
		delete[] mbuf;

		/*
		 * Validate signature
		 */

		XSECProvider prov;
		DSIGSignature * sig = prov.newSignatureFromDOM(doc);
		sig->load();
		sig->setSigningKey(k);

		bool ret = sig->verify();

		doc->release();

		return ret;

	}

	catch (XSECException &e)
	{
		cerr << "An error occured during signature processing\n   Message: ";
		char * ce = XMLString::transcode(e.getMsg());
		cerr << ce << endl;
		delete ce;
		exit(1);
		
	}	
	catch (XSECCryptoException &e)
	{
		cerr << "A cryptographic error occured during signature processing\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}

}
// --------------------------------------------------------------------------------
//           Unit tests for signature
// --------------------------------------------------------------------------------


void unitTestEnvelopingSignature(DOMImplementation * impl) {
	
	// This tests an enveloping signature as the root node

	cerr << "Creating enveloping signature ... ";
	
	try {
		
		// Create a document
    
		DOMDocument * doc = impl->createDocument();

		// Create the signature

		XSECProvider prov;
		DSIGSignature *sig;
		DOMElement *sigNode;
		
		sig = prov.newSignature();
		sig->setDSIGNSPrefix(MAKE_UNICODE_STRING("ds"));
		sig->setPrettyPrint(true);

		sigNode = sig->createBlankSignature(doc, CANON_C14N_COM, SIGNATURE_HMAC, HASH_SHA1);

		doc->appendChild(sigNode);

		// Add an object
		DSIGObject * obj = sig->appendObject();
		obj->setId(MAKE_UNICODE_STRING("ObjectId"));

		// Create a text node
		DOMText * txt= doc->createTextNode(MAKE_UNICODE_STRING("A test string"));
		obj->appendChild(txt);

		// Add a Reference
		sig->createReference(MAKE_UNICODE_STRING("#ObjectId"));

		// Get a key
		cerr << "signing ... ";

		sig->setSigningKey(createHMACKey((unsigned char *) "secret"));
		sig->sign();

		cerr << "validating ... ";
		if (!sig->verify()) {
			cerr << "bad verify!" << endl;
			exit(1);
		}

		cerr << "OK ... serialise and re-verify ... ";
		if (!reValidateSig(impl, doc, createHMACKey((unsigned char *) "secret"))) {

			cerr << "bad verify!" << endl;
			exit(1);

		}

		cerr << "OK ... ";

		// Now set to bad
		txt->setNodeValue(MAKE_UNICODE_STRING("A bad string"));

		cerr << "verify bad data ... ";
		if (sig->verify()) {

			cerr << "bad - should have failed!" << endl;
			exit(1);

		}

		cerr << "OK (verify false) ... serialise and re-verify ... ";
		if (reValidateSig(impl, doc, createHMACKey((unsigned char *) "secret"))) {

			cerr << "bad - should have failed" << endl;
			exit(1);

		}

		cerr << "OK" << endl;
		// Reset to OK
		txt->setNodeValue(MAKE_UNICODE_STRING("A test string"));
		outputDoc(impl, doc);
		doc->release();
		

	}

	catch (XSECException &e)
	{
		cerr << "An error occured during signature processing\n   Message: ";
		char * ce = XMLString::transcode(e.getMsg());
		cerr << ce << endl;
		delete ce;
		exit(1);
		
	}	
	catch (XSECCryptoException &e)
	{
		cerr << "A cryptographic error occured during signature processing\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}


}

void unitTestBase64NodeSignature(DOMImplementation * impl) {
	
	// This tests a normal signature with a reference to a Base64 element

	cerr << "Creating a base64 Element reference ... ";
	
	try {
		
		// Create a document
    
		DOMDocument * doc = impl->createDocument();

		// Create the signature

		XSECProvider prov;
		DSIGSignature *sig;
		DOMElement *sigNode;
		
		sig = prov.newSignature();
		sig->setDSIGNSPrefix(MAKE_UNICODE_STRING("ds"));
		sig->setPrettyPrint(true);
		sig->setIdByAttributeName(false);		// Do not search by name

		sigNode = sig->createBlankSignature(doc, CANON_C14N_COM, SIGNATURE_HMAC, HASH_SHA1);

		doc->appendChild(sigNode);

		// Add an object
		DSIGObject * obj = sig->appendObject();
		obj->setId(MAKE_UNICODE_STRING("ObjectId"));

		// Create a text node
		DOMText * txt= doc->createTextNode(MAKE_UNICODE_STRING("QSB0ZXN0IHN0cmluZw=="));
		obj->appendChild(txt);

		// Add a Reference
		DSIGReference * ref = sig->createReference(MAKE_UNICODE_STRING("#ObjectId"));
		// Add a Base64 transform
		ref->appendBase64Transform();

		// Get a key
		cerr << "signing ... ";

		sig->setSigningKey(createHMACKey((unsigned char *) "secret"));
		sig->sign();

		cerr << "validating ... ";
		if (!sig->verify()) {
			cerr << "bad verify!" << endl;
			exit(1);
		}

		cerr << "OK ... serialise and re-verify ... ";
		if (!reValidateSig(impl, doc, createHMACKey((unsigned char *) "secret"))) {

			cerr << "bad verify!" << endl;
			exit(1);

		}

		cerr << "OK ... ";

		// Now set to bad
		txt->setNodeValue(MAKE_UNICODE_STRING("QSAybmQgdGVzdCBzdHJpbmc="));

		cerr << "verify bad data ... ";
		if (sig->verify()) {

			cerr << "bad - should have failed!" << endl;
			exit(1);

		}

		cerr << "OK (verify false) ... serialise and re-verify ... ";
		if (reValidateSig(impl, doc, createHMACKey((unsigned char *) "secret"))) {

			cerr << "bad - should have failed" << endl;
			exit(1);

		}

		cerr << "OK" << endl;
		// Reset to OK
		txt->setNodeValue(MAKE_UNICODE_STRING("QSB0ZXN0IHN0cmluZw=="));
		outputDoc(impl, doc);
		doc->release();
		

	}

	catch (XSECException &e)
	{
		cerr << "An error occured during signature processing\n   Message: ";
		char * ce = XMLString::transcode(e.getMsg());
		cerr << ce << endl;
		delete ce;
		exit(1);
		
	}	
	catch (XSECCryptoException &e)
	{
		cerr << "A cryptographic error occured during signature processing\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}


}


void unitTestSignature(DOMImplementation * impl) {

	// Test an enveloping signature
	unitTestEnvelopingSignature(impl);
#ifndef XSEC_NO_XALAN
	unitTestBase64NodeSignature(impl);
#else
	cerr << "Skipping base64 node test (Requires XPath)" << endl;
#endif
}

// --------------------------------------------------------------------------------
//           Basic tests of signature function
// --------------------------------------------------------------------------------

void testSignature(DOMImplementation *impl) {

	cerr << "Creating a known doc and signing (HMAC-SHA1)" << endl;
	
	// Create a document
    
	DOMDocument * doc = createTestDoc(impl);

	// Check signature functions

	XSECProvider prov;
	DSIGSignature *sig;
	DSIGReference *ref[10];
	DOMElement *sigNode;
	int refCount;

	try {
		
		/*
		 * Now we have a document, create a signature for it.
		 */
		
		sig = prov.newSignature();
		sig->setDSIGNSPrefix(MAKE_UNICODE_STRING("ds"));
		sig->setPrettyPrint(true);

		sigNode = sig->createBlankSignature(doc, CANON_C14N_COM, SIGNATURE_HMAC, HASH_SHA1);
		DOMElement * rootElem = doc->getDocumentElement();
		DOMNode * prodElem = rootElem->getFirstChild();

		rootElem->appendChild(doc->createTextNode(DSIGConstants::s_unicodeStrNL));
		rootElem->insertBefore(doc->createComment(MAKE_UNICODE_STRING(" a comment ")), prodElem);
		rootElem->appendChild(sigNode);
		rootElem->insertBefore(doc->createTextNode(DSIGConstants::s_unicodeStrNL), prodElem);

		/*
		 * Add some test references
		 */

		ref[0] = sig->createReference(MAKE_UNICODE_STRING(""));
		ref[0]->appendEnvelopedSignatureTransform();

		ref[1] = sig->createReference(MAKE_UNICODE_STRING("#xpointer(/)"));
		ref[1]->appendEnvelopedSignatureTransform();
		ref[1]->appendCanonicalizationTransform(CANON_C14N_NOC);

		ref[2] = sig->createReference(MAKE_UNICODE_STRING("#xpointer(/)"));
		ref[2]->appendEnvelopedSignatureTransform();
		ref[2]->appendCanonicalizationTransform(CANON_C14N_COM);

		ref[3] = sig->createReference(MAKE_UNICODE_STRING("#xpointer(/)"));
		ref[3]->appendEnvelopedSignatureTransform();
		ref[3]->appendCanonicalizationTransform(CANON_C14NE_NOC);

		ref[4] = sig->createReference(MAKE_UNICODE_STRING("#xpointer(/)"));
		ref[4]->appendEnvelopedSignatureTransform();
		ref[4]->appendCanonicalizationTransform(CANON_C14NE_COM);

		ref[5] = sig->createReference(MAKE_UNICODE_STRING("#xpointer(/)"));
		ref[5]->appendEnvelopedSignatureTransform();
		DSIGTransformC14n * ce = ref[5]->appendCanonicalizationTransform(CANON_C14NE_COM);
		ce->addInclusiveNamespace("foo");

		sig->setECNSPrefix(MAKE_UNICODE_STRING("ec"));
		ref[6] = sig->createReference(MAKE_UNICODE_STRING("#xpointer(/)"));
		ref[6]->appendEnvelopedSignatureTransform();
		ce = ref[6]->appendCanonicalizationTransform(CANON_C14NE_COM);
		ce->addInclusiveNamespace("foo");

#ifdef XSEC_NO_XALAN

		cerr << "WARNING : No testing of XPath being performed as Xalan not present" << endl;
		refCount = 7;

#else
		/*
		 * Create some XPath/XPathFilter references
		 */


		ref[7] = sig->createReference(MAKE_UNICODE_STRING(""));
		sig->setXPFNSPrefix(MAKE_UNICODE_STRING("xpf"));
		DSIGTransformXPathFilter * xpf = ref[7]->appendXPathFilterTransform();
		xpf->appendFilter(FILTER_INTERSECT, MAKE_UNICODE_STRING("//ADoc/category"));

		ref[8] = sig->createReference(MAKE_UNICODE_STRING(""));
		/*		ref[5]->appendXPathTransform("ancestor-or-self::dsig:Signature", 
				"xmlns:dsig=http://www.w3.org/2000/09/xmldsig#"); */

		DSIGTransformXPath * x = ref[8]->appendXPathTransform("count(ancestor-or-self::dsig:Signature | \
here()/ancestor::dsig:Signature[1]) > \
count(ancestor-or-self::dsig:Signature)");
		x->setNamespace("dsig", "http://www.w3.org/2000/09/xmldsig#");

		refCount = 9;

#endif
	
		/*
		 * Sign the document, using an HMAC algorithm and the key "secret"
		 */


		sig->appendKeyName(MAKE_UNICODE_STRING("The secret key is \"secret\""));

		// Append a test DNames

		DSIGKeyInfoX509 * x509 = sig->appendX509Data();
		x509->setX509SubjectName(s_tstDName);

		// Append a test PGPData element
		sig->appendPGPData(s_tstPGPKeyID, s_tstPGPKeyPacket);

		// Append an SPKIData element
		DSIGKeyInfoSPKIData * spki = sig->appendSPKIData(s_tstSexp1);
		spki->appendSexp(s_tstSexp2);

		// Append a MgmtData element
		sig->appendMgmtData(s_tstMgmtData);

		sig->setSigningKey(createHMACKey((unsigned char *) "secret"));
		sig->sign();

		// Output the document post signature if necessary
		outputDoc(impl, doc);

		cerr << endl << "Doc signed OK - Checking values against Known Good" << endl;

		unsigned char buf[128];
		int len;

		/*
		 * Validate the reference hash values from known good
		 */

		int i;
		for (i = 0; i < refCount; ++i) {

			cerr << "Calculating hash for reference " << i << " ... ";

			len = (int) ref[i]->calculateHash(buf, 128);

			cerr << " Done\nChecking -> ";

			if (len != 20) {
				cerr << "Bad (Length = " << len << ")" << endl;
				exit (1);
			}

			for (int j = 0; j < 20; ++j) {

				if (buf[j] != createdDocRefs[i][j]) {
					cerr << "Bad at location " << j << endl;
					exit (1);
				}
			
			}
			cerr << "Good.\n";

		}

		/*
		 * Verify the signature check works
		 */

		cerr << "Running \"verifySignatureOnly()\" on calculated signature ... ";
		if (sig->verifySignatureOnly()) {
			cerr << "OK" << endl;
		}
		else {
			cerr << "Failed" << endl;
			char * e = XMLString::transcode(sig->getErrMsgs());
			cout << e << endl;
			XSEC_RELEASE_XMLCH(e);
			exit(1);
		}

		/*
		 * Change the document and ensure the signature fails.
		 */

		cerr << "Setting incorrect key in Signature object" << endl;
		sig->setSigningKey(createHMACKey((unsigned char *) "badsecret"));

		cerr << "Running \"verifySignatureOnly()\" on calculated signature ... ";
		if (!sig->verifySignatureOnly()) {
			cerr << "OK (Signature bad)" << endl;
		}
		else {
			cerr << "Failed (signature OK but should be bad)" << endl;
			exit(1);
		}

		// Don't need the signature now the DOM structure is in place
		prov.releaseSignature(sig);

		/*
		 * Now serialise the document to memory so we can re-parse and check from scratch
		 */

		cerr << "Serialising the document to a memory buffer ... ";

		DOMWriter         *theSerializer = ((DOMImplementationLS*)impl)->createDOMWriter();

		theSerializer->setEncoding(MAKE_UNICODE_STRING("UTF-8"));
		if (theSerializer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, false))
			theSerializer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, false);


		MemBufFormatTarget *formatTarget = new MemBufFormatTarget();

		theSerializer->writeNode(formatTarget, *doc);

		// Copy to a new buffer
		len = formatTarget->getLen();
		char * mbuf = new char [len + 1];
		memcpy(mbuf, formatTarget->getRawBuffer(), len);
		mbuf[len] = '\0';
#if 0
		cout << mbuf << endl;
#endif
		delete theSerializer;
		delete formatTarget;

		cerr << "done\nParsing memory buffer back to DOM ... ";

		// Also release the document so that we can re-load from scratch

		doc->release();

		/*
		 * Re-parse
		 */

		XercesDOMParser parser;
		
		parser.setDoNamespaces(true);
		parser.setCreateEntityReferenceNodes(true);

		MemBufInputSource* memIS = new MemBufInputSource ((const XMLByte*) mbuf, 
																len, "XSECMem");

		parser.parse(*memIS);
		doc = parser.adoptDocument();


		delete(memIS);
		delete[] mbuf;

		cerr << "done\nValidating signature ...";

		/*
		 * Validate signature
		 */

		sig = prov.newSignatureFromDOM(doc);
		sig->load();
		sig->setSigningKey(createHMACKey((unsigned char *) "secret"));

		if (sig->verify()) {
			cerr << "OK" << endl;
		}
		else {
			cerr << "Failed\n" << endl;
			char * e = XMLString::transcode(sig->getErrMsgs());
			cerr << e << endl;
			XSEC_RELEASE_XMLCH(e);
			exit(1);
		}

		/*
		 * Ensure DNames are read back in and decoded properly
		 */

		DSIGKeyInfoList * kil = sig->getKeyInfoList();
		int nki = (int) kil->getSize();

		cerr << "Checking Distinguished name is decoded correctly ... ";
		for (i = 0; i < nki; ++i) {

			if (kil->item(i)->getKeyInfoType() == DSIGKeyInfo::KEYINFO_X509) {

				if (strEquals(s_tstDName, ((DSIGKeyInfoX509 *) kil->item(i))->getX509SubjectName())) {
					cerr << "yes" << endl;
				}
				else {
					cerr << "decoded incorrectly" << endl;;
					exit (1);
				}
			}
			if (kil->item(i)->getKeyInfoType() == DSIGKeyInfo::KEYINFO_PGPDATA) {
				
				cerr << "Validating PGPData read back OK ... ";

				DSIGKeyInfoPGPData * p = (DSIGKeyInfoPGPData *)kil->item(i);

				if (!(strEquals(p->getKeyID(), s_tstPGPKeyID) &&
					strEquals(p->getKeyPacket(), s_tstPGPKeyPacket))) {

					cerr << "no!";
					exit(1);
				}

				cerr << "yes\n";
			}
			if (kil->item(i)->getKeyInfoType() == DSIGKeyInfo::KEYINFO_SPKIDATA) {
				
				cerr << "Validating SPKIData read back OK ... ";

				DSIGKeyInfoSPKIData * s = (DSIGKeyInfoSPKIData *)kil->item(i);

				if (s->getSexpSize() != 2) {
					cerr << "no - expected two S-expressions";
					exit(1);
				}

				if (!(strEquals(s->getSexp(0), s_tstSexp1) &&
					strEquals(s->getSexp(1), s_tstSexp2))) {

					cerr << "no!";
					exit(1);
				}

				cerr << "yes\n";
			}
			if (kil->item(i)->getKeyInfoType() == DSIGKeyInfo::KEYINFO_MGMTDATA) {
				
				cerr << "Validating MgmtData read back OK ... ";

				DSIGKeyInfoMgmtData * m = (DSIGKeyInfoMgmtData *)kil->item(i);

				if (!strEquals(m->getData(), s_tstMgmtData)) {

					cerr << "no!";
					exit(1);
				}

				cerr << "yes\n";
			}
		}
	}

	catch (XSECException &e)
	{
		cerr << "An error occured during signature processing\n   Message: ";
		char * ce = XMLString::transcode(e.getMsg());
		cerr << ce << endl;
		delete ce;
		exit(1);
		
	}	
	catch (XSECCryptoException &e)
	{
		cerr << "A cryptographic error occured during signature processing\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}

	// Output the document post signature if necessary
	outputDoc(impl, doc);

	doc->release();

}

// --------------------------------------------------------------------------------
//           Unit tests for test encrypt/Decrypt
// --------------------------------------------------------------------------------

void unitTestCipherReference(DOMImplementation * impl) {

	DOMDocument *doc = impl->createDocument(
				0,                    // root element namespace URI.
				MAKE_UNICODE_STRING("ADoc"),            // root element name
				NULL);// DOMDocumentType());  // document type object (DTD).

	DOMElement *rootElem = doc->getDocumentElement();

	// Use key k to wrap a test key, decrypt it and make sure it is still OK
	XSECProvider prov;
	XENCCipher * cipher;

	try {

		cipher = prov.newCipher(doc);

		cerr << "Creating CipherReference ... ";

		XENCEncryptedData * xenc = 
			cipher->createEncryptedData(XENCCipherData::REFERENCE_TYPE, DSIGConstants::s_unicodeStrURIAES128_CBC, MAKE_UNICODE_STRING("#CipherText"));

		rootElem->appendChild(xenc->getElement());

		// Now create the data that is referenced
		DOMElement * cipherVal = doc->createElement(MAKE_UNICODE_STRING("MyCipherValue"));
		rootElem->appendChild(cipherVal);
		cipherVal->setAttribute(MAKE_UNICODE_STRING("Id"), MAKE_UNICODE_STRING("CipherText"));
#if defined(XSEC_XERCES_HAS_SETIDATTRIBUTE)
		cipherVal->setIdAttribute(MAKE_UNICODE_STRING("Id"));
#endif

		cipherVal->appendChild(doc->createTextNode(MAKE_UNICODE_STRING((char *) s_tstBase64EncodedString)));

		// Now add the transforms necessary to decrypt
		XENCCipherReference *cref = xenc->getCipherData()->getCipherReference();

		if (cref == NULL) {
			cerr << "Failed - no CipherReference object" << endl;
			exit(1);
		}

		cerr << "done ... appending XPath and Base64 transforms ... ";

		//cref->appendXPathTransform("self::text()[parent::rep:CipherValue[@Id="example1"]]");
		cref->appendXPathTransform("self::text()[parent::MyCipherValue[@Id=\"CipherText\"]]");
		cref->appendBase64Transform();

		cerr << "done ... decrypting ... ";

		// Create a key
		XSECCryptoSymmetricKey * ks =
				XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_128);
		ks->setKey((unsigned char *) s_keyStr, 16);

		cipher->setKey(ks);

		// Now try to decrypt
		DOMNode * n = findXENCNode(doc, "EncryptedData");

		XSECBinTXFMInputStream *is = cipher->decryptToBinInputStream((DOMElement *) n);
		Janitor<XSECBinTXFMInputStream> j_is(is);

		XMLByte buf[1024];

		cerr << "done ... comparing to known good ... ";

		int bytesRead = is->readBytes(buf, 1024);
		buf[bytesRead] = '\0';
		if (strcmp((char *) buf, s_tstDecryptedString) == 0) {
			cerr << "OK" << endl;
		}
		else {
			cerr << "failed - bad compare of decrypted data" << endl;
		}

	}

	catch (XSECException &e)
	{
		cerr << "failed\n";
		cerr << "An error occured during signature processing\n   Message: ";
		char * ce = XMLString::transcode(e.getMsg());
		cerr << ce << endl;
		delete ce;
		exit(1);
		
	}	
	catch (XSECCryptoException &e)
	{
		cerr << "failed\n";
		cerr << "A cryptographic error occured during signature processing\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}

	outputDoc(impl, doc);
	doc->release();

}


void unitTestElementContentEncrypt(DOMImplementation *impl, XSECCryptoKey * key, encryptionMethod em, bool doElementContent) {

	if (doElementContent)
		cerr << "Encrypting Element Content ... ";
	else
		cerr << "Encrypting Element ... ";
	
	// Create a document
    
	DOMDocument * doc = createTestDoc(impl);
	DOMNode * categoryNode = findNode(doc, MAKE_UNICODE_STRING("category"));
	if (categoryNode == NULL) {

		cerr << "Error finding category node for encryption test" << endl;
		exit(1);

	}

	// Create and execute cipher

	XSECProvider prov;
	XENCCipher * cipher;

	try {
		
		/*
		 * Now we have a document, find the data node.
		 */

		cipher = prov.newCipher(doc);
		cipher->setXENCNSPrefix(MAKE_UNICODE_STRING("xenc"));
		cipher->setPrettyPrint(true);

		// Set a key

		cipher->setKey(key->clone());
	
		// Now encrypt!
		if (doElementContent)
			cipher->encryptElementContent(doc->getDocumentElement(), em);
		else
			cipher->encryptElement((DOMElement *) categoryNode, em);

		cerr << "done ... check encrypted ... ";

		DOMNode * t = findNode(doc, MAKE_UNICODE_STRING("category"));
		if (t != NULL) {

			cerr << "no - a category child still exists" << endl;
			exit(1);

		}
		else
			cerr << "yes" << endl;
		
		outputDoc(impl, doc);
		
		if (doElementContent)
			cerr << "Decrypting Element content ... ";
		else
			cerr << "Decrypting Element ... ";

		// OK - Now we try to decrypt
		// Find the EncryptedData node
		DOMNode * n = findXENCNode(doc, "EncryptedData");

		XENCCipher * cipher2 = prov.newCipher(doc);

		cipher2->setKey(key);

		cipher2->decryptElement(static_cast<DOMElement *>(n));

		cerr << "done ... check decrypt ... ";
		t = findNode(doc, MAKE_UNICODE_STRING("category"));

		if (t == NULL) {

			cerr << " failed - category did not decrypt properly" << endl;
			exit(1);

		}
		else
			cerr << "OK" << endl;

		outputDoc(impl, doc);

	}
	catch (XSECException &e)
	{
		cerr << "An error occured during encryption processing\n   Message: ";
		char * ce = XMLString::transcode(e.getMsg());
		cerr << ce << endl;
		delete ce;
		exit(1);
		
	}	
	catch (XSECCryptoException &e)
	{
		cerr << "A cryptographic error occured during encryption processing\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}

	doc->release();

}


void unitTestKeyEncrypt(DOMImplementation *impl, XSECCryptoKey * k, encryptionMethod em) {

	// Create a document that we will embed the encrypted key in
	DOMDocument *doc = impl->createDocument(
				0,                    // root element namespace URI.
				MAKE_UNICODE_STRING("ADoc"),            // root element name
				NULL);// DOMDocumentType());  // document type object (DTD).

	DOMElement *rootElem = doc->getDocumentElement();

	// Use key k to wrap a test key, decrypt it and make sure it is still OK
	XSECProvider prov;
	XENCCipher * cipher;

	try {
		
		// Encrypt a dummy key

		cerr << "encrypt ... ";

		static unsigned char toEncryptStr[] = "A test key to use for da";

		cipher = prov.newCipher(doc);
		cipher->setXENCNSPrefix(MAKE_UNICODE_STRING("xenc"));
		cipher->setPrettyPrint(true);

		// Set a key

		cipher->setKEK(k);

		XENCEncryptedKey * encryptedKey;
		encryptedKey = cipher->encryptKey(toEncryptStr, (unsigned int) strlen((char *) toEncryptStr), em);
		Janitor<XENCEncryptedKey> j_encryptedKey(encryptedKey);

		rootElem->appendChild(encryptedKey->getElement());

		// Decrypt
		cerr << "decrypt ... ";

		XMLByte decBuf[64];
		cipher->decryptKey(encryptedKey, decBuf, 64);

		// Check
		cerr << "comparing ... ";
		if (memcmp(decBuf, toEncryptStr, strlen((char *) toEncryptStr)) == 0) {
			cerr << "OK ... ";
		}
		else {
			cerr << "different = failed!" << endl;
			exit(2);
		}
		
		cerr << "decrypt from DOM ... ";
		// Decrypt from DOM
		DOMNode * keyNode = findXENCNode(doc, "EncryptedKey");
		if (keyNode == NULL) {
			cerr << "no key - failed!" << endl;
			exit(2);
		}
		memset(decBuf, 0, 64);
		cipher->decryptKey((DOMElement *) keyNode, decBuf, 64);

		cerr << "comparing ... ";
		if (memcmp(decBuf, toEncryptStr, strlen((char *) toEncryptStr)) == 0) {
			cerr << "OK" << endl;
		}
		else {
			cerr << "different = failed!" << endl;
			exit(2);
		}

	}

	catch (XSECException &e)
	{
		cerr << "failed\n";
		cerr << "An error occured during signature processing\n   Message: ";
		char * ce = XMLString::transcode(e.getMsg());
		cerr << ce << endl;
		delete ce;
		exit(1);
		
	}	
	catch (XSECCryptoException &e)
	{
		cerr << "failed\n";
		cerr << "A cryptographic error occured during signature processing\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}

	outputDoc(impl, doc);
	doc->release();

}



void unitTestEncrypt(DOMImplementation *impl) {

	try {
		// Key wraps
		cerr << "RSA key wrap... ";

#if defined (HAVE_OPENSSL) && defined (HAVE_WINCAPI)
		if (!g_useWinCAPI) {
#endif

#if defined (HAVE_OPENSSL)
		// Load the key
		BIO * bioMem = BIO_new(BIO_s_mem());
		BIO_puts(bioMem, s_tstRSAPrivateKey);
		EVP_PKEY * pk = PEM_read_bio_PrivateKey(bioMem, NULL, NULL, NULL);

		OpenSSLCryptoKeyRSA * k = new OpenSSLCryptoKeyRSA(pk);

		unitTestKeyEncrypt(impl, k, ENCRYPT_RSA_15);

		cerr << "RSA OAEP key wrap... ";
		k = new OpenSSLCryptoKeyRSA(pk);
		unitTestKeyEncrypt(impl, k, ENCRYPT_RSA_OAEP_MGFP1);

		cerr << "RSA OAEP key wrap + params... ";
		k = new OpenSSLCryptoKeyRSA(pk);
		k->setOAEPparams(s_tstOAEPparams, (unsigned int) strlen((char *) s_tstOAEPparams));

		unitTestKeyEncrypt(impl, k, ENCRYPT_RSA_OAEP_MGFP1);

		BIO_free(bioMem);
		EVP_PKEY_free(pk);

#endif

#if defined (HAVE_OPENSSL) && defined (HAVE_WINCAPI)
		} else {
#endif

#if defined (HAVE_WINCAPI)

		// Use the internal key
		WinCAPICryptoProvider *cp = dynamic_cast<WinCAPICryptoProvider *>(XSECPlatformUtils::g_cryptoProvider);
		HCRYPTPROV p = cp->getApacheKeyStore();
		
		WinCAPICryptoKeyRSA * rsaKey = new WinCAPICryptoKeyRSA(p, AT_KEYEXCHANGE, true);
		unitTestKeyEncrypt(impl, rsaKey, ENCRYPT_RSA_15);

		cerr << "RSA OAEP key wrap... ";
		rsaKey = new WinCAPICryptoKeyRSA(p, AT_KEYEXCHANGE, true);
		unitTestKeyEncrypt(impl, rsaKey, ENCRYPT_RSA_OAEP_MGFP1);


#endif

#if defined (HAVE_OPENSSL) && defined (HAVE_WINCAPI)
		}
#endif

		XSECCryptoSymmetricKey * ks;

		if (g_haveAES) {
			cerr << "AES 128 key wrap... ";

			ks = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_128);
			ks->setKey((unsigned char *) s_keyStr, 16);
		
			unitTestKeyEncrypt(impl, ks, ENCRYPT_KW_AES128);

			cerr << "AES 192 key wrap... ";

			ks = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_192);
			ks->setKey((unsigned char *) s_keyStr, 24);
		
			unitTestKeyEncrypt(impl, ks, ENCRYPT_KW_AES192);

			cerr << "AES 256 key wrap... ";

			ks = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_256);
			ks->setKey((unsigned char *) s_keyStr, 32);
		
			unitTestKeyEncrypt(impl, ks, ENCRYPT_KW_AES256);
		}

		else 
			cerr << "Skipped AES key wrap tests" << endl;

		cerr << "Triple DES key wrap... ";

		ks = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_3DES_192);
		ks->setKey((unsigned char *) s_keyStr, 24);
		
		unitTestKeyEncrypt(impl, ks, ENCRYPT_KW_3DES);

		// Now do Element encrypts

		if (g_haveAES) {
			// 128 AES
			ks = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_128);
			ks->setKey((unsigned char *) s_keyStr, 16);

			cerr << "Unit testing AES 128 bit CBC encryption" << endl;
			unitTestElementContentEncrypt(impl, ks->clone(), ENCRYPT_AES128_CBC, false);
			unitTestElementContentEncrypt(impl, ks, ENCRYPT_AES128_CBC, true);

			//192 AES
			ks = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_192);
			ks->setKey((unsigned char *) s_keyStr, 24);

			cerr << "Unit testing AES 192 bit CBC encryption" << endl;
			unitTestElementContentEncrypt(impl, ks->clone(), ENCRYPT_AES192_CBC, false);
			unitTestElementContentEncrypt(impl, ks, ENCRYPT_AES192_CBC, true);

		// 256 AES
			ks = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_256);
			ks->setKey((unsigned char *) s_keyStr, 32);

			cerr << "Unit testing AES 256 bit CBC encryption" << endl;
			unitTestElementContentEncrypt(impl, ks->clone(), ENCRYPT_AES256_CBC, false);
			unitTestElementContentEncrypt(impl, ks, ENCRYPT_AES256_CBC, true);
		}

		else
			cerr << "Skipped AES Element tests" << endl;

		// 192 3DES
		ks = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_3DES_192);
		ks->setKey((unsigned char *) s_keyStr, 24);

		cerr << "Unit testing 3DES CBC encryption" << endl;
		unitTestElementContentEncrypt(impl, ks->clone(), ENCRYPT_3DES_CBC, false);
		unitTestElementContentEncrypt(impl, ks, ENCRYPT_3DES_CBC, true);
#ifndef XSEC_NO_XALAN
		if (g_haveAES) {
			cerr << "Unit testing CipherReference creation and decryption" << endl;
			unitTestCipherReference(impl);
		}
		else {
			cerr << "Skipped Cipher Reference Test (uses AES)" << endl;
		}
#else
		cerr << "Skipped Cipher Reference Test (requires XPath)" << endl;
#endif

	}
	catch (XSECCryptoException &e)
	{
		cerr << "failed\n";
		cerr << "A cryptographic error occured during encryption unit tests\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}

}
// --------------------------------------------------------------------------------
//           Test encrypt/Decrypt
// --------------------------------------------------------------------------------

void testEncrypt(DOMImplementation *impl) {

	cerr << "Creating a known doc encrypting a portion of it" << endl;
	
	// Create a document
    
	DOMDocument * doc = createTestDoc(impl);
	DOMNode * categoryNode = findNode(doc, MAKE_UNICODE_STRING("category"));
	if (categoryNode == NULL) {

		cerr << "Error finding category node for encryption test" << endl;
		exit(1);

	}

	// Check signature functions

	XSECProvider prov;
	XENCCipher * cipher;

	try {
		
		/*
		 * Now we have a document, find the data node.
		 */

		// Generate a key
		unsigned char randomBuffer[256];

		if (XSECPlatformUtils::g_cryptoProvider->getRandom(randomBuffer, 256) != 256) {

			cerr << "Unable to obtain enough random bytes from Crypto Provider" << endl;
			exit(1);
		
		}

		cipher = prov.newCipher(doc);
		cipher->setXENCNSPrefix(MAKE_UNICODE_STRING("xenc"));
		cipher->setPrettyPrint(true);

		// Set a key

		XSECCryptoSymmetricKey * k = 
			XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_3DES_192);
		k->setKey((unsigned char *) randomBuffer, 24);
		cipher->setKey(k);
	
		// Now encrypt!
		cerr << "Performing 3DES encryption on <category> element ... ";
		cipher->encryptElement((DOMElement *) categoryNode, ENCRYPT_3DES_CBC);

		// Add a KeyInfo
		cerr << "done\nAppending a <KeyName> ... ";
		XENCEncryptedData * encryptedData = cipher->getEncryptedData();
		encryptedData->appendKeyName(s_tstKeyName);
		cerr << "done\nAdding Encoding and MimeType ... ";

		// Add MimeType and Encoding
		encryptedData->setEncoding(s_tstEncoding);
		encryptedData->setMimeType(s_tstMimeType);

		// Set a KeySize
		cerr << "done\nSetting <KeySize> ... ";
		encryptedData->getEncryptionMethod()->setKeySize(192);

		cerr << "done\nSearching for <category> ... ";

		DOMNode * t = findNode(doc, MAKE_UNICODE_STRING("category"));
		if (t != NULL) {

			cerr << "found!\nError - category is not encrypted" << endl;
			exit(1);

		}
		else
			cerr << "not found (OK - now encrypted)" << endl;

		// Now try to encrypt the Key

		cerr << "Encrypting symmetric key ... " << endl;

		XSECCryptoSymmetricKey * kek;
		if (g_haveAES) {

			kek = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_128);
			kek->setKey((unsigned char *) s_keyStr, 16);
		}
		else {
			kek = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_3DES_192);
			kek->setKey((unsigned char *) s_keyStr, 24);

		}
		
		cipher->setKEK(kek);

		XENCEncryptedKey * encryptedKey;
		if (g_haveAES)
			encryptedKey = cipher->encryptKey(randomBuffer, 24, ENCRYPT_KW_AES128);
		else
			encryptedKey = cipher->encryptKey(randomBuffer, 24, ENCRYPT_KW_3DES);
		cerr << "done!" << endl;

		cerr << "Adding CarriedKeyName and Recipient to encryptedKey ... " << endl;
		encryptedKey->setCarriedKeyName(s_tstCarriedKeyName);
		encryptedKey->setRecipient(s_tstRecipient);
		cerr << "done!" << endl;

		encryptedData->appendEncryptedKey(encryptedKey);

		outputDoc(impl, doc);

		// OK - Now we try to decrypt
		// Find the EncryptedData node
		DOMNode * n = findXENCNode(doc, "EncryptedData");

		XENCCipher * cipher2 = prov.newCipher(doc);

		XSECCryptoSymmetricKey * k2;
		
		if (g_haveAES) {

			k2 = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_AES_128);
			k2->setKey((unsigned char *) s_keyStr, 16);
		}
		
		else {
			k2 = XSECPlatformUtils::g_cryptoProvider->keySymmetric(XSECCryptoSymmetricKey::KEY_3DES_192);
			k2->setKey((unsigned char *) s_keyStr, 24);

		}

		cipher2->setKEK(k2);

		cerr << "Decrypting ... ";
		cipher2->decryptElement(static_cast<DOMElement *>(n));
		cerr << "done" << endl;

		cerr << "Checking for <category> element ... ";

		t = findNode(doc, MAKE_UNICODE_STRING("category"));

		if (t == NULL) {

			cerr << " not found!\nError - category did not decrypt properly" << endl;
			exit(1);

		}
		else
			cerr << "found" << endl;

		cerr << "Checking <KeyName> element is set correctly ... ";

		encryptedData = cipher2->getEncryptedData();

		if (encryptedData == NULL) {
			cerr << "no - cannot access EncryptedData element" << endl;
			exit(1);
		}

		DSIGKeyInfoList * kil = encryptedData->getKeyInfoList();
		int nki = (int) kil->getSize();
		bool foundNameOK = false;

		int i;
		for (i = 0; i < nki; ++i) {

			if (kil->item(i)->getKeyInfoType() == DSIGKeyInfo::KEYINFO_NAME) {

				DSIGKeyInfoName *n = dynamic_cast<DSIGKeyInfoName *>(kil->item(i));
				if (!strEquals(n->getKeyName(), s_tstKeyName)) {
					
					cerr << "no!" << endl;
					exit (1);
				}
				foundNameOK = true;
				break;
			}

		}

		if (foundNameOK == false) {
			cerr << "no!" << endl;
			exit(1);
		}
		else
			cerr << "yes." << endl;

		cerr << "Checking CarriedKeyName and Recipient values ... ";
		bool foundCCN = false;
		bool foundRecipient = false;

		for (i = 0; i < nki; ++i) {

			if (kil->item(i)->getKeyInfoType() == DSIGKeyInfo::KEYINFO_ENCRYPTEDKEY) {

				XENCEncryptedKey * xek = dynamic_cast<XENCEncryptedKey*>(kil->item(i));

				if (strEquals(xek->getCarriedKeyName(), s_tstCarriedKeyName)) {

					foundCCN = true;
				}
				
				if (strEquals(xek->getRecipient(), s_tstRecipient)) {

					foundRecipient = true;
				}
			}
		}

		if (foundCCN == false || foundRecipient == false) {
			cerr << "no!" << endl;
			exit(1);
		}
		else {
			cerr << "OK" << endl;
		}

		cerr << "Checking MimeType and Encoding ... ";
		if (encryptedData->getMimeType() == NULL || !strEquals(encryptedData->getMimeType(), s_tstMimeType)) {
			cerr << "Bad MimeType" << endl;
			exit(1);
		}
		if (encryptedData->getEncoding() == NULL || !strEquals(encryptedData->getEncoding(), s_tstEncoding)) {
			cerr << "Bad Encoding" << endl;
			exit(1);
		}

		cerr << "OK" << endl;

		cerr << "Checking KeySize in EncryptionMethod ... ";
		if (encryptedData->getEncryptionMethod() == NULL || encryptedData->getEncryptionMethod()->getKeySize() != 192) {
			cerr << "Bad KeySize" << endl;
			exit(1);
		}

		cerr << "OK" << endl;

	}
	catch (XSECException &e)
	{
		cerr << "An error occured during signature processing\n   Message: ";
		char * ce = XMLString::transcode(e.getMsg());
		cerr << ce << endl;
		delete ce;
		exit(1);
		
	}	
	catch (XSECCryptoException &e)
	{
		cerr << "A cryptographic error occured during signature processing\n   Message: "
		<< e.getMsg() << endl;
		exit(1);
	}

	outputDoc(impl, doc);
	doc->release();

}

// --------------------------------------------------------------------------------
//           Test XKMS basics
// --------------------------------------------------------------------------------

void testXKMS(DOMImplementation *impl) {

	// This is really a place holder

	cerr << "Making POST call to server ...  " << endl;
	
	// Create a document
    
	DOMDocument * doc = createTestDoc(impl);
	DOMNode * categoryNode = findNode(doc, MAKE_UNICODE_STRING("category"));

	/*
	XSECSOAPRequestorSimpleWin32 req(MAKE_UNICODE_STRING("http://zeus/post.php"));

	req.doRequest(doc);
	*/

	doc->release();
}

	
// --------------------------------------------------------------------------------
//           Print usage instructions
// --------------------------------------------------------------------------------

void printUsage(void) {

	cerr << "\nUsage: xtest [options]\n\n";
	cerr << "     Where options are :\n\n";
	cerr << "     --help/-h\n";
	cerr << "         This help message\n\n";
#if defined (HAVE_WINCAPI)  && defined (HAVE_OPENSSL)
	cerr << "     --wincapi/-w\n";
	cerr << "         Use Windows Crypto API for crypto functionality\n\n";
#endif
	cerr << "     --print-docs/-p\n";
	cerr << "         Print the test documents\n\n";
	cerr << "     --signature-only/-s\n";
	cerr << "         Only run basic signature test\n\n";
	cerr << "     --signature-unit-only/-t\n";
	cerr << "         Only run signature unit tests\n\n";
	cerr << "     --encryption-only/-e\n";
	cerr << "         Only run basic encryption test\n\n";
	cerr << "     --encryption-unit-only/-u\n";
	cerr << "         Only run encryption unit tests\n\n";
	cerr << "     --xkms-only/-x\n";
	cerr << "         Only run basic XKMS test\n\n";

}
// --------------------------------------------------------------------------------
//           Main
// --------------------------------------------------------------------------------

int main(int argc, char **argv) {

	/* We output a version number to overcome a "feature" in Microsoft's memory
	   leak detection */

	cerr << "DSIG Info (Using Apache XML-Security-C Library v" << XSEC_VERSION_MAJOR <<
		"." << XSEC_VERSION_MEDIUM << "." << XSEC_VERSION_MINOR << ")\n";

	// Check parameters
	bool		doEncryptionTest = true;
	bool		doEncryptionUnitTests = true;
	bool		doSignatureTest = true;
	bool		doSignatureUnitTests = true;
	bool		doXKMSTest = true;

	int paramCount = 1;

	while (paramCount < argc) {

		if (stricmp(argv[paramCount], "--help") == 0 || stricmp(argv[paramCount], "-h") == 0) {
			printUsage();
			exit(0);
		}
		else if (stricmp(argv[paramCount], "--print-docs") == 0 || stricmp(argv[paramCount], "-p") == 0) {
			g_printDocs = true;
			paramCount++;
		}
#if defined(HAVE_WINCAPI) && defined(HAVE_OPENSSL)
		else if (stricmp(argv[paramCount], "--wincapi") == 0 || stricmp(argv[paramCount], "-w") == 0) {
			g_useWinCAPI = true;
			paramCount++;
		}
#endif
		else if (stricmp(argv[paramCount], "--signature-only") == 0 || stricmp(argv[paramCount], "-s") == 0) {
			doEncryptionTest = false;
			doEncryptionUnitTests = false;
			doSignatureUnitTests = false;
			doXKMSTest = false;
			paramCount++;
		}
		else if (stricmp(argv[paramCount], "--encryption-only") == 0 || stricmp(argv[paramCount], "-e") == 0) {
			doSignatureTest = false;
			doEncryptionUnitTests = false;
			doSignatureUnitTests = false;
			doXKMSTest = false;
			paramCount++;
		}
		else if (stricmp(argv[paramCount], "--encryption-unit-only") == 0 || stricmp(argv[paramCount], "-u") == 0) {
			doEncryptionTest = false;
			doSignatureTest = false;
			doSignatureUnitTests = false;
			doXKMSTest = false;
			paramCount++;
		}
		else if (stricmp(argv[paramCount], "--signature-unit-only") == 0 || stricmp(argv[paramCount], "-t") == 0) {
			doEncryptionTest = false;
			doSignatureTest = false;
			doEncryptionUnitTests = false;
			doXKMSTest = false;
			paramCount++;
		}
		else if (stricmp(argv[paramCount], "--xkms-only") == 0 || stricmp(argv[paramCount], "-x") == 0) {
			doEncryptionTest = false;
			doSignatureTest = false;
			doEncryptionUnitTests = false;
			doSignatureUnitTests = false;
			paramCount++;
		}
		else {
			printUsage();
			return 2;
		}
	}


#if defined (_DEBUG) && defined (_MSC_VER)

	// Do some memory debugging under Visual C++

	_CrtMemState s1, s2, s3;

	// At this point we are about to start really using XSEC, so
	// Take a "before" checkpoing

	_CrtMemCheckpoint( &s1 );

#endif

	// First initialise the XML system

	try {

		XMLPlatformUtils::Initialize();
#ifndef XSEC_NO_XALAN
		XPathEvaluator::initialize();
		XalanTransformer::initialize();
#endif
		XSECPlatformUtils::Initialise();

#if defined (HAVE_OPENSSL) && defined (HAVE_WINCAPI)
		if (g_useWinCAPI) {
			// Setup for Windows Crypt API
			WinCAPICryptoProvider * cp;
			// First set windows as the crypto provider
			cp = new WinCAPICryptoProvider();
			XSECPlatformUtils::SetCryptoProvider(cp);
		}
#endif


	}
	catch (const XMLException &e) {

		cerr << "Error during initialisation of Xerces" << endl;
		cerr << "Error Message = : "
		     << e.getMessage() << endl;

	}

	{

		// Set up for tests

		g_haveAES = XSECPlatformUtils::g_cryptoProvider->algorithmSupported(XSECCryptoSymmetricKey::KEY_AES_128);

		// Setup for building documents

		XMLCh tempStr[100];
		XMLString::transcode("Core", tempStr, 99);    
		DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(tempStr);

		// Output some info
		char * provName = XMLString::transcode(XSECPlatformUtils::g_cryptoProvider->getProviderName());
		cerr << "Crypto Provider string : " << provName << endl;
		XSEC_RELEASE_XMLCH(provName);

		// Test signature functions
		if (doSignatureTest) {
			cerr << endl << "====================================";
			cerr << endl << "Testing Signature Function";
			cerr << endl << "====================================";
			cerr << endl << endl;

			testSignature(impl);
		}

		// Test signature functions
		if (doSignatureUnitTests) {
			cerr << endl << "====================================";
			cerr << endl << "Performing Signature Unit Tests";
			cerr << endl << "====================================";
			cerr << endl << endl;

			unitTestSignature(impl);
		}

		// Test encrypt function
		if (doEncryptionTest) {
			cerr << endl << "====================================";
			cerr << endl << "Testing Encryption Function";
			cerr << endl << "====================================";
			cerr << endl << endl;

			testEncrypt(impl);
		}

		// Running Encryption Unit test
		if (doEncryptionUnitTests) {
			cerr << endl << "====================================";
			cerr << endl << "Performing Encryption Unit Tests";
			cerr << endl << "====================================";
			cerr << endl << endl;

			unitTestEncrypt(impl);
		}

		// Running XKMS Base test
		if (doXKMSTest) {
			cerr << endl << "====================================";
			cerr << endl << "Performing XKMS Function";
			cerr << endl << "====================================";
			cerr << endl << endl;

			testXKMS(impl);
		}

		cerr << endl << "All tests passed" << endl;

	}

	XSECPlatformUtils::Terminate();
#ifndef XSEC_NO_XALAN
	XalanTransformer::terminate();
	XPathEvaluator::terminate();
#endif
	XMLPlatformUtils::Terminate();

#if defined (_DEBUG) && defined (_MSC_VER)

	_CrtMemCheckpoint( &s2 );

	if ( _CrtMemDifference( &s3, &s1, &s2 ) && (
		s3.lCounts[0] > 0 ||
		s3.lCounts[1] > 1 ||
		// s3.lCounts[2] > 2 ||  We don't worry about C Runtime
		s3.lCounts[3] > 0 ||
		s3.lCounts[4] > 0)) {

		// Note that there is generally 1 Normal and 1 CRT block
		// still taken.  1 is from Xalan and 1 from stdio

		// Send all reports to STDOUT
		_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
		_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
		_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
		_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
		_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );

		// Dumpy memory stats

 		_CrtMemDumpAllObjectsSince( &s3 );
	    _CrtMemDumpStatistics( &s3 );
	}

	// Now turn off memory leak checking and end as there are some 
	// Globals that are allocated that get seen as leaks (Xalan?)

	int dbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	dbgFlag &= ~(_CRTDBG_LEAK_CHECK_DF);
	_CrtSetDbgFlag( dbgFlag );

#endif


	return 0;

}