/**@file
	@brief Mobility Management messages, GSM 04.08 9.2.
*/
/*
* Copyright 2008 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/



#ifndef GSML3MMMESSAGES_H
#define GSML3MMMESSAGES_H

#include "GSMCommon.h"
#include "GSML3Message.h"
#include "GSML3CommonElements.h"
#include "GSML3MMElements.h"



namespace GSM {



/**
	This a virtual class for L3 messages in the Mobility Management protocol.
	These messages are defined in GSM 04.08 9.2
*/

class L3MMMessage : public L3Message {

	public:

	/**
		Message type codes for mobility management messages in L3, GSM 04.08, Table 10.2.
		Note that bit 6 (MSB-less-1) is a "don't care" for these codes.
	*/
	enum MessageType {
		IMSIDetachIndication=0x01,
		CMServiceAccept=0x21,
		CMServiceReject=0x22,
		CMServiceAbort=0x23,
		CMServiceRequest=0x24,
		CMReestablishmentRequest=0x28,
		IdentityResponse=0x19,
		IdentityRequest=0x18,
		MMInformation=0x32,
		LocationUpdatingAccept=0x02,
		LocationUpdatingReject=0x04,
		LocationUpdatingRequest=0x08,
		TMSIReallocationCommand=0x1a,
		MMStatus=0x31,
		Undefined=-1
	};


	L3MMMessage():L3Message() {}

	/** Return the L3 protocol discriptor. */
	L3PD PD() const { return L3MobilityManagementPD; }

	void text(std::ostream&) const;
};



std::ostream& operator<<(std::ostream& os, L3MMMessage::MessageType val);



/**
	A Factory function to return a L3MMMessage of the specified MTI.
	Returns NULL if the MTI is not supported.
*/
L3MMMessage* L3MMFactory(L3MMMessage::MessageType MTI);

/**
	Parse a complete L3 mobility management message into its object type.
	@param source The L3 bits.
	@return A pointer to a new message or NULL on failure.
*/
L3MMMessage* parseL3MM(const L3Frame& source);




/** GSM 04.08 9.2.15 */
class L3LocationUpdatingRequest : public L3MMMessage
{
	// For now, we are ignoring everything but the mobile identity.
	L3MobileIdentity mMobileIdentity; // (LV) 1+len
	L3LocationAreaIdentity mLAI;

public:
	L3LocationUpdatingRequest():L3MMMessage() {}

	const L3MobileIdentity& mobileIdentity() const
		{ return mMobileIdentity; }
	const L3LocationAreaIdentity& LAI() const
		{ return mLAI; }

	int MTI() const { return (int)LocationUpdatingRequest; }
	
	size_t bodyLength() const;
	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody( const L3Frame &src, size_t &rp );	
	void text(std::ostream&) const;
};



/** GSM 04.08 9.2.13 */
class L3LocationUpdatingAccept : public L3MMMessage
{
	// LAI = (V) length of 6
	L3LocationAreaIdentity mLAI;
	bool mHaveMobileIdentity;
	L3MobileIdentity mMobileIdentity;
	
public:

	L3LocationUpdatingAccept(const L3LocationAreaIdentity& wLAI)
		:L3MMMessage(),mLAI(wLAI),
		mHaveMobileIdentity(false)
	{}

	L3LocationUpdatingAccept(
		const L3LocationAreaIdentity& wLAI,
		const L3MobileIdentity& wMobileIdentity)
		:L3MMMessage(),mLAI(wLAI),
		mHaveMobileIdentity(true),
		mMobileIdentity(wMobileIdentity)
	{}

	int MTI() const { return (int)LocationUpdatingAccept; }
	
	size_t bodyLength() const;
	void writeBody( L3Frame &src, size_t &rp ) const;
	void parseBody( const L3Frame& src, size_t &rp ) { abort(); }
	void text(std::ostream&) const;

};


class L3MMStatus : public L3MMMessage
{
	L3RejectCause mRejectCause;
	
public:
	L3MMStatus() : L3MMMessage(){}

	int MTI() const { return (int) MMStatus; }

	size_t bodyLength() const { return 3;  }
	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody( const L3Frame& src, size_t &rp );
	void text(std::ostream&) const;

};




/** GSM 04.08 9.2.14  */
class L3LocationUpdatingReject : public L3MMMessage 
{
	L3RejectCause mRejectCause;

public:

	L3LocationUpdatingReject(const L3RejectCause& cause)
		:L3MMMessage(),mRejectCause(cause)
	{}

	int MTI() const { return (int)LocationUpdatingReject; }
	
	size_t bodyLength() const { return mRejectCause.lengthV(); }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame& src, size_t &rp ) { abort(); }
	void text(std::ostream&) const;

};	



/** GSM 04.08 9.2.12 */
class L3IMSIDetachIndication : public L3MMMessage {

	private:

	// NOTE -- There's a classmark element here that we ignore.

	L3MobileIdentity mMobileIdentity;

	public:

	const L3MobileIdentity& mobileIdentity() const
		{ return mMobileIdentity; }

	int MTI() const { return (int)IMSIDetachIndication; }

	size_t bodyLength() const { return 1 + mMobileIdentity.lengthLV(); }
	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody( const L3Frame &src, size_t &rp );
	void text(std::ostream&) const;

};


/** GSM 04.08 9.2.5 */
class L3CMServiceAccept : public L3MMMessage {

	public:

	int MTI() const { return (int)CMServiceAccept; }

	size_t bodyLength() const { return 0; }
	void writeBody( L3Frame &dest, size_t &wp ) const {}
	void parseBody( const L3Frame &src, size_t &rp ) { abort(); }
};



/** GSM 04.08 9.2.7 */
class L3CMServiceAbort : public L3MMMessage {

	public:

	int MTI() const { return (int)CMServiceAbort; }

	size_t bodyLength() const { return 0; }
	void writeBody( L3Frame &dest, size_t &wp ) const {}
	void parseBody( const L3Frame &src, size_t &rp );
};



/** GSM 04.08 9.2.6 */
class L3CMServiceReject : public L3MMMessage {

	private:

	L3RejectCause mCause;

	public:

	L3CMServiceReject(const L3RejectCause& wCause)
		:L3MMMessage(),
		mCause(wCause)
	{}

	int MTI() const { return (int)CMServiceReject; }

	size_t bodyLength() const { return mCause.lengthV(); }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp ) { abort(); }
	void text(std::ostream&) const;
};


/** GSM 04.08 9.2.9 */
class L3CMServiceRequest : public L3MMMessage 
{
	L3MobileIdentity mMobileIdentity;
	L3CMServiceType mServiceType;

public:

	L3CMServiceRequest() 
		:L3MMMessage(),
		mServiceType()
	{ }

	/** Accessors */
	//@{
	const L3CMServiceType& serviceType() const { return mServiceType; }

	const L3MobileIdentity& mobileIdentity() const
		{ return mMobileIdentity; }
	//@}

	int MTI() const { return (int)CMServiceRequest; }

	// (1/2) + (1/2) + 4 + 
	size_t bodyLength() const { return 5+mMobileIdentity.lengthLV(); }

	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody( const L3Frame &src, size_t &rp );
	void text(std::ostream&) const;
};



/** GSM 04.08 9.2.4 */
class L3CMReestablishmentRequest : public L3MMMessage {

	private:

	L3MobileIdentity mMobileID;
	bool mHaveLAI;
	L3LocationAreaIdentity mLAI;

	public:

	L3CMReestablishmentRequest()
		:L3MMMessage(),
		mHaveLAI(false)
	{}

	const L3MobileIdentity& mobileID() const { return mMobileID; }

	int MTI() const { return (int)CMReestablishmentRequest; }

	size_t bodyLength() const { return 1 + 4 + mMobileID.lengthLV(); }
	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody( const L3Frame &src, size_t &rp );
	void text(std::ostream&) const;
};


/** GSM 04.08 9.2.15a */
class L3MMInformation : public L3MMMessage {

	private:

	const L3NetworkName& mShortName;	///< This is normally taken from gBTS gloal configuration.

	public:

	/**
		Constructor.
		@param wShortName Abbreviated network name, MUST PERSIST.
	*/
	L3MMInformation(const L3NetworkName& wShortName)
		:L3MMMessage(),
		mShortName(wShortName)
	{}

	int MTI() const { return (int)MMInformation; }

	size_t bodyLength() const;
	void writeBody(L3Frame&,size_t&) const;
	void parseBody( const L3Frame &src, size_t &rp ) { abort(); }
	void text(std::ostream&) const;
};


/** Identity Request, GSM 04.08 9.2.10 */
class L3IdentityRequest : public L3MMMessage {

	private:
	
	MobileIDType mType;

	public:

	L3IdentityRequest(MobileIDType wType)
		:L3MMMessage(),mType(wType)
	{}

	int MTI() const { return IdentityRequest; }

	size_t bodyLength() const { return 1; }
	void writeBody(L3Frame& dest, size_t& wp) const;
	void parseBody( const L3Frame &src, size_t &rp ) { abort(); }
	void text(std::ostream&) const;
};



/** Identity Response, GSM 04.08 9.2.11 */
class L3IdentityResponse : public L3MMMessage {

	private :

	L3MobileIdentity mMobileID;

	public:

	const L3MobileIdentity& mobileID() const { return mMobileID; }

	int MTI() const { return IdentityResponse; }

	size_t bodyLength() const { return mMobileID.lengthLV(); }
	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody( const L3Frame &src, size_t &rp );
	void text(std::ostream&) const;
};

};	// namespace GSM

#endif //#ifndef GSML3MM_H

// vim: ts=4 sw=4